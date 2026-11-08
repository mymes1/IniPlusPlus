// Numeric INI-backed store for the five AssArray ACEs used by Sonic Classic 2.
//
// This implements the flat key=value form in ringdata.sav from Sonic Classic 1. That file is
// only a format fixture and does not establish Sonic Classic 2 save compatibility. It does not
// emulate the original AssArray String map, binary array files, encryption, or unrelated ACEs.
package Extensions;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.io.Writer;
import java.nio.charset.StandardCharsets;
import java.util.LinkedHashMap;
import java.util.Locale;
import java.util.Map;

final class AssArrayIniStore {
    private static final int MAX_FILE_BYTES = 16 * 1024 * 1024;

    private final File filesDirectory;
    private final LinkedHashMap<String, Double> values = new LinkedHashMap<String, Double>();

    AssArrayIniStore(File appFilesDirectory) {
        filesDirectory = appFilesDirectory;
    }

    void clear() {
        values.clear();
    }

    void set(String key, double value) {
        String normalizedKey = normalizeKey(key);
        if (normalizedKey.length() == 0 || Double.isNaN(value) || Double.isInfinite(value)) {
            return;
        }
        values.put(normalizedKey, Double.valueOf(value));
    }

    double get(String key) {
        Double value = values.get(normalizeKey(key));
        return value == null ? 0.0 : value.doubleValue();
    }

    boolean load(String requestedPath) {
        try {
            File file = resolvePath(requestedPath);
            if (file == null || !file.isFile()) {
                return false;
            }

            String text = readText(file);
            LinkedHashMap<String, Double> loaded = new LinkedHashMap<String, Double>();
            boolean sawSection = false;
            boolean sawAssignment = false;
            boolean sawOtherContent = false;
            String[] lines = text.split("\\r\\n|\\n|\\r", -1);
            for (String rawLine : lines) {
                String line = rawLine == null ? "" : rawLine.trim();
                if (line.length() == 0 || line.charAt(0) == ';' || line.charAt(0) == '#') {
                    continue;
                }
                if (line.charAt(0) == '[' && line.charAt(line.length() - 1) == ']') {
                    sawSection = true;
                    continue;
                }

                int equals = line.indexOf('=');
                if (equals < 0) {
                    sawOtherContent = true;
                    continue;
                }
                sawAssignment = true;
                String key = line.substring(0, equals).trim();
                String numberText = line.substring(equals + 1).trim();
                String normalizedKey = normalizeKey(key);
                if (normalizedKey.length() == 0) {
                    continue;
                }
                Double number = parseNumber(numberText);
                if (number != null) {
                    loaded.put(normalizedKey, number);
                }
            }

            // Reject opaque or non-numeric data rather than treating it as an empty save and
            // clearing the in-memory array. A valid empty file/comment-only file remains loadable.
            if (loaded.isEmpty() && (sawAssignment || sawOtherContent)
                    && !sawSection && text.trim().length() != 0) {
                return false;
            }
            values.clear();
            values.putAll(loaded);
            return true;
        } catch (IOException | RuntimeException exception) {
            return false;
        }
    }

    boolean save(String requestedPath) {
        try {
            File file = resolvePath(requestedPath);
            if (file == null) {
                return false;
            }
            File parent = file.getParentFile();
            if (parent != null && !parent.isDirectory() && !parent.mkdirs() && !parent.isDirectory()) {
                return false;
            }
            // Re-resolve after directory creation so a newly encountered symlink cannot escape
            // filesDirectory between the initial path check and the write.
            file = resolvePath(requestedPath);
            if (file == null) {
                return false;
            }

            Writer writer = new OutputStreamWriter(new FileOutputStream(file), StandardCharsets.UTF_8);
            try {
                for (Map.Entry<String, Double> entry : values.entrySet()) {
                    writer.write(entry.getKey());
                    writer.write('=');
                    writer.write(formatNumber(entry.getValue().doubleValue()));
                    writer.write("\r\n");
                }
                writer.flush();
            } finally {
                writer.close();
            }
            return true;
        } catch (IOException | RuntimeException exception) {
            return false;
        }
    }

    private File resolvePath(String requestedPath) throws IOException {
        if (filesDirectory == null || requestedPath == null) {
            return null;
        }
        String name = requestedPath.trim().replace('\\', '/');
        if (name.length() == 0 || name.indexOf('\0') >= 0) {
            return null;
        }

        // Fusion projects often build paths from a Windows drive expression. Strip the drive and
        // treat the remainder as app-private relative storage (C:\\Game\\ringdata.sav ->
        // <filesDir>/Game/ringdata.sav).
        if (name.length() >= 2 && isAsciiLetter(name.charAt(0)) && name.charAt(1) == ':') {
            name = name.substring(2);
            while (name.startsWith("/")) {
                name = name.substring(1);
            }
        }

        File root = filesDirectory.getCanonicalFile();
        File requested = new File(name);
        File resolved;
        if (requested.isAbsolute()) {
            resolved = requested.getCanonicalFile();
            if (!isWithin(root, resolved)) {
                return null;
            }
        } else {
            resolved = new File(root, name).getCanonicalFile();
        }
        return isWithin(root, resolved) ? resolved : null;
    }

    private static boolean isWithin(File root, File path) {
        String rootPath = root.getPath();
        String pathName = path.getPath();
        return pathName.equals(rootPath) || pathName.startsWith(rootPath + File.separator);
    }

    private static String readText(File file) throws IOException {
        FileInputStream input = new FileInputStream(file);
        ByteArrayOutputStream output = new ByteArrayOutputStream();
        try {
            byte[] buffer = new byte[8192];
            int total = 0;
            int count;
            while ((count = input.read(buffer)) != -1) {
                total += count;
                if (total > MAX_FILE_BYTES) {
                    throw new IOException("INI file is too large");
                }
                output.write(buffer, 0, count);
            }
        } finally {
            input.close();
        }
        byte[] bytes = output.toByteArray();
        for (byte value : bytes) {
            if (value == 0) {
                throw new IOException("binary file is not a flat INI save");
            }
        }
        String text = new String(bytes, StandardCharsets.UTF_8);
        if (text.length() > 0 && text.charAt(0) == '\ufeff') {
            text = text.substring(1);
        }
        return text;
    }

    private static Double parseNumber(String text) {
        if (text == null || text.length() == 0) {
            return null;
        }
        try {
            double value = Double.parseDouble(text);
            if (Double.isNaN(value) || Double.isInfinite(value)) {
                return null;
            }
            return Double.valueOf(value);
        } catch (NumberFormatException exception) {
            return null;
        }
    }

    private static String normalizeKey(String key) {
        if (key == null) {
            return "";
        }
        String normalized = key.trim();
        if (normalized.length() == 0 || normalized.indexOf('=') >= 0
                || normalized.indexOf('\r') >= 0 || normalized.indexOf('\n') >= 0
                || normalized.indexOf('\0') >= 0) {
            return "";
        }
        return normalized.toLowerCase(Locale.ROOT);
    }

    private static String formatNumber(double value) {
        if (value >= Long.MIN_VALUE && value <= Long.MAX_VALUE) {
            long integerValue = (long) value;
            if ((double) integerValue == value) {
                return Long.toString(integerValue);
            }
        }
        return Double.toString(value);
    }

    private static boolean isAsciiLetter(char value) {
        return (value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z');
    }
}
