package Extensions;

import java.io.File;
import java.io.FileOutputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;

public final class AssArrayIniStoreTests {
    private static void require(boolean condition, String message) {
        if (!condition) {
            throw new AssertionError(message);
        }
    }

    public static void main(String[] args) throws Exception {
        if (args.length != 1) {
            throw new IllegalArgumentException("usage: AssArrayIniStoreTests <ringdata.sav>");
        }

        File sample = new File(args[0]).getCanonicalFile();
        require(sample.isFile(), "ringdata.sav sample must exist");
        File filesDirectory = Files.createTempDirectory("assarray-ini-test-").toFile();
        File copiedSample = new File(filesDirectory, "ringdata.sav");
        Files.copy(sample.toPath(), copiedSample.toPath(), StandardCopyOption.REPLACE_EXISTING);

        AssArrayIniStore store = new AssArrayIniStore(filesDirectory);
        require(store.load("ringdata.sav"), "load the supplied flat INI save");
        require(store.get("RING1392") == 1.0, "case-insensitive GetValue finds a saved lowercase ring key");
        require(store.get("ring4533") == 1.0, "load the final key from the sample");
        require(store.get("ring999999") == 0.0, "a missing key returns zero");

        store.set("RING9999", 1.0);
        require(store.save("roundtrip.sav"), "save a round-trip file");
        byte[] savedBytes = Files.readAllBytes(new File(filesDirectory, "roundtrip.sav").toPath());
        String savedText = new String(savedBytes, StandardCharsets.UTF_8);
        require(!savedText.startsWith("["), "save output is flat and has no section header");
        require(savedText.contains("ring9999=1\r\n"), "save output uses lowercase keys and integer formatting");

        AssArrayIniStore roundTrip = new AssArrayIniStore(filesDirectory);
        require(roundTrip.load("roundtrip.sav"), "reload the saved round-trip file");
        require(roundTrip.get("RING9999") == 1.0, "round-trip retains numeric values and case-insensitive keys");

        File opaque = new File(filesDirectory, "opaque.sav");
        try (FileOutputStream output = new FileOutputStream(opaque)) {
            output.write(new byte[] {0x16, (byte) 0xf4, (byte) 0x80, (byte) 0xb3, 0x39, (byte) 0xd5});
        }
        require(!roundTrip.load("opaque.sav"), "unrecognized binary data is rejected");
        require(roundTrip.get("RING9999") == 1.0, "rejected data does not clear current values");

        require(!roundTrip.save("../escape.sav"), "parent traversal is rejected");
        System.out.println("AssArray INI store tests passed against " + sample.getName());
    }

    private AssArrayIniStoreTests() {
    }
}
