# Turns the proprietary Clickteam Android extension SDK header into the single-line base64 string the
# CI workflow expects as the repository secret CLICKTEAM_ANDROID_SDK_B64 - the Windows version of
# tools/make_sdk_b64.sh.
#
#   powershell -ExecutionPolicy Bypass -File tools\make_sdk_b64.ps1 -SdkDir C:\sdk\clickteam-android
#   powershell -ExecutionPolicy Bypass -File tools\make_sdk_b64.ps1 -Zip C:\sdk\clickteam-android-sdk.zip
#
# Availability note: the current Gradle archive checked for this project lacks RuntimeNative.h;
# no complete current public native C++ SDK has been verified. Use this helper ONLY after obtaining
# the authorized, compatible C++/NDK SDK/header from Clickteam. It cannot turn a Gradle-only archive
# into a native C++ SDK. See docs/BUILD.md, section 2. The header is searched for recursively.
#
# Then:
#   gh secret set CLICKTEAM_ANDROID_SDK_B64 --repo <owner>/<repo> < clickteam-android-sdk.b64
# or web UI: Settings -> Secrets and variables -> Actions -> New repository secret.
#
# The header is proprietary and must never be committed, published or shipped inside INI++.zip.
[CmdletBinding(DefaultParameterSetName = 'Dir')]
param(
	[Parameter(ParameterSetName = 'Dir', Position = 0)]
	[string]$SdkDir = $env:IPPP_ANDROID_SDK_DIR,

	[Parameter(ParameterSetName = 'Zip')]
	[string]$Zip,

	[string]$Out = 'clickteam-android-sdk.b64',

	# Print the base64 to the console as well (it is long - useful for pasting into the web UI).
	[switch]$Print
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Fail([string]$message) {
	Write-Error $message
	exit 1
}

# Reject the repository's stand-ins before they can end up in a secret.
function Test-Header([string]$path) {
	$text = Get-Content -Raw -LiteralPath $path
	if ($text -match 'IPPP_MOCK_RUNTIME_NATIVE_H') {
		Fail "$path is android/tests/mock-sdk/RuntimeNative.h, the host-test stand-in - not the Clickteam SDK header."
	}
	if ($text -match 'RuntimeNative_Shim_HeaderPlusPlus') {
		Fail "$path is android/sdk-shim/RuntimeNative.h, the documented reconstruction - not the Clickteam SDK header."
	}
	if ($text -notmatch 'RuntimeFunctions') {
		Fail "$path does not mention RuntimeFunctions, so it is probably not the header."
	}
}

if (-not $SdkDir -and -not $Zip) {
	Fail @"
usage: tools\make_sdk_b64.ps1 -SdkDir C:\path\to\extracted-sdk [-Out clickteam-android-sdk.b64] [-Print]
       tools\make_sdk_b64.ps1 -Zip C:\path\to\existing.zip     [-Out clickteam-android-sdk.b64] [-Print]

Point -SdkDir at the extracted Clickteam Android extension SDK (docs\BUILD.md, section 2),
or -Zip at a zip that already contains RuntimeNative.h.
"@
}

$work = Join-Path ([IO.Path]::GetTempPath()) ("ippsdk-" + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $work | Out-Null
try {
	$header = $null
	if ($Zip) {
		if (-not (Test-Path -LiteralPath $Zip -PathType Leaf)) { Fail "$Zip does not exist." }
		$unpacked = Join-Path $work 'unpacked'
		Expand-Archive -LiteralPath $Zip -DestinationPath $unpacked
		$header = Get-ChildItem -LiteralPath $unpacked -Recurse -Filter 'RuntimeNative.h' -File |
			Select-Object -First 1
		if (-not $header) { Fail "$Zip contains no RuntimeNative.h." }
		Test-Header $header.FullName
		Copy-Item -LiteralPath $Zip -Destination (Join-Path $work 'sdk.zip')
		Write-Host "Found the header in $Zip : $($header.FullName.Substring($unpacked.Length + 1))"
	} else {
		if (-not (Test-Path -LiteralPath $SdkDir -PathType Container)) { Fail "$SdkDir is not a directory." }
		$header = Get-ChildItem -LiteralPath $SdkDir -Recurse -Filter 'RuntimeNative.h' -File -Depth 6 |
			Select-Object -First 1
		if (-not $header) {
			Fail "$SdkDir contains no RuntimeNative.h - is that really the extracted Clickteam Android extension SDK?  See docs\BUILD.md, section 2."
		}
		Test-Header $header.FullName
		$root = (Resolve-Path -LiteralPath $SdkDir).Path.TrimEnd('\', '/')
		$rel = $header.FullName.Substring($root.Length).TrimStart('\', '/')
		$relDir = Split-Path -Parent $rel

		# The header plus the other headers in its directory (it may #include them), nothing else:
		# GitHub rejects repository secrets larger than 48 kB.
		$stage = Join-Path $work 'stage'
		$destDir = if ($relDir) { Join-Path $stage $relDir } else { $stage }
		New-Item -ItemType Directory -Path $destDir -Force | Out-Null
		$headers = Get-ChildItem -LiteralPath $header.DirectoryName -File |
			Where-Object { $_.Extension -in '.h', '.hpp' }
		foreach ($file in $headers) { Copy-Item -LiteralPath $file.FullName -Destination $destDir }
		Write-Host "Found the header: $rel (packing $($headers.Count) header file(s) from its directory)"

		$zipPath = Join-Path $work 'sdk.zip'
		Add-Type -AssemblyName System.IO.Compression, System.IO.Compression.FileSystem
		$archive = [IO.Compression.ZipFile]::Open($zipPath, 'Create')
		try {
			foreach ($file in Get-ChildItem -LiteralPath $stage -Recurse -File) {
				$entryName = $file.FullName.Substring($stage.Length).TrimStart('\', '/').Replace('\', '/')
				[void][IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
					$archive, $file.FullName, $entryName, [IO.Compression.CompressionLevel]::Optimal)
			}
		} finally {
			$archive.Dispose()
		}
	}

	$zipBytes = (Get-Item -LiteralPath (Join-Path $work 'sdk.zip')).Length
	$base64 = [Convert]::ToBase64String([IO.File]::ReadAllBytes((Join-Path $work 'sdk.zip')))
	Set-Content -LiteralPath $Out -Value $base64 -NoNewline -Encoding ascii

	Write-Host "zip:     $zipBytes bytes"
	Write-Host "base64:  $($base64.Length) characters -> $Out"
	if ($base64.Length -gt 48000) {
		Write-Warning "$($base64.Length) characters is close to or over GitHub's 48 kB repository secret limit. This script packs only the header's directory; if that is still too big, pack just RuntimeNative.h itself and check whether the build needs any header it includes."
	}
	Write-Host ''
	Write-Host 'Put it into the repository secret (never into the repository):'
	Write-Host '  gh secret set CLICKTEAM_ANDROID_SDK_B64 --repo <owner>/<repo> < clickteam-android-sdk.b64'
	Write-Host 'or web UI: Settings -> Secrets and variables -> Actions -> New repository secret'
	if ($Print) { Write-Host ''; Write-Host $base64 }
} finally {
	Remove-Item -LiteralPath $work -Recurse -Force -ErrorAction SilentlyContinue
}
