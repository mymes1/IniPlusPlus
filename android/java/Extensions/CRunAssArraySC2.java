// Android runtime for the separate ASSARRAYSC2 extension object.
// The ACE IDs/signatures match the five operations used by the Sonic Classic 2 event set; this class
// does not replace or depend on the original ASSARRAY runtime.
package Extensions;

import java.io.File;

import Actions.CActExtension;
import Conditions.CCndExtension;
import Expressions.CValue;
import Objects.CExtension;
import RunLoop.CCreateObjectInfo;
import Services.CBinaryFile;

public final class CRunAssArraySC2 extends CRunExtension {
    private static final int CONDITION_COUNT = 12;

    // Stable ACE IDs shared with the five operations in the Sonic Classic 2 event set.
    private static final int ACTION_SET_VALUE_AT_KEY = 0;
    private static final int ACTION_CLEAR_ARRAY = 3;
    private static final int ACTION_LOAD_INI_INTO_VALUES = 12;
    private static final int ACTION_SAVE_INI_FROM_VALUES = 14;
    private static final int EXPRESSION_GET_VALUE = 0;

    private AssArraySC2IniStore store;

    @Override
    public int getNumberOfConditions() {
        // Preserve the original condition table size. The inspected Sonic Classic 2 export does
        // not use AssArray conditions; the unimplemented conditions safely return false.
        return CONDITION_COUNT;
    }

    @Override
    public boolean createRunObject(CBinaryFile file, CCreateObjectInfo cob, int version) {
        File filesDirectory = null;
        CExtension extensionObject = ho;
        if (extensionObject != null && extensionObject.getControlsContext() != null) {
            filesDirectory = extensionObject.getControlsContext().getFilesDir();
        }
        store = new AssArraySC2IniStore(filesDirectory);
        return false;
    }

    @Override
    public int handleRunObject() {
        return REFLAG_ONESHOT;
    }

    @Override
    public void destroyRunObject(boolean fast) {
        store = null;
    }

    @Override
    public boolean condition(int id, CCndExtension cnd) {
        // Conditions are outside this five-ACE target.
        return false;
    }

    @Override
    public void action(int id, CActExtension act) {
        if (store == null || act == null) {
            return;
        }

        switch (id) {
            case ACTION_SET_VALUE_AT_KEY: {
                String key = act.getParamExpString(rh, 0);
                double value = act.getParamExpDouble(rh, 1);
                store.set(key, value);
                break;
            }
            case ACTION_CLEAR_ARRAY:
                store.clear();
                break;
            case ACTION_LOAD_INI_INTO_VALUES:
                store.load(act.getParamExpString(rh, 0));
                break;
            case ACTION_SAVE_INI_FROM_VALUES:
                store.save(act.getParamExpString(rh, 0));
                break;
            default:
                // Other actions are outside the five-operation replacement target.
                break;
        }
    }

    @Override
    public CValue expression(int id) {
        if (store == null || id != EXPRESSION_GET_VALUE) {
            return new CValue(0);
        }
        CValue keyParameter = ho == null ? null : ho.getExpParam();
        String key = keyParameter == null ? "" : keyParameter.getString();
        return new CValue(store.get(key));
    }
}
