#pragma once

#include <filesystem>
#include <optional>
#include <string_view>

namespace ipp_android
{
	void log(std::string_view text) noexcept;

	// Use the Java extension's Context.getFilesDir() value as the app-private root for both the
	// shared INI implementation and the Fusion default-folder properties.
	void set_data_dir(std::filesystem::path directory) noexcept;
	[[nodiscard]] std::filesystem::path data_dir();
	[[nodiscard]] std::optional<std::filesystem::path> safe_data_path(std::filesystem::path path);
}
