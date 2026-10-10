// Ini++ Unicode Android adapter for the Fusion 2.5 public Java CRunExtension API.
//
// The class is registered from the exporter-side CExtLoad.java integration hook (see
// tools/integrate_android_exporter.sh). JNI calls the project-owned Bridge.cpp; this is not a
// Clickteam RuntimeNative extension and does not require RuntimeNative.h.
package Extensions;

import Actions.CActExtension;
import Conditions.CCndExtension;
import Expressions.CValue;
import Objects.CExtension;
import Params.CPositionInfo;
import RunLoop.CCreateObjectInfo;
import Services.CBinaryFile;

public final class CRunIniPlusPlus extends CRunExtension {
    private long nativeHandle;

    @Override
    public int getNumberOfConditions() {
        return nativeGetNumberOfConditions();
    }

    @Override
    public boolean createRunObject(CBinaryFile file, CCreateObjectInfo cob, int version) {
        byte[] editData = file == null || file.data == null ? new byte[0] : file.data;
        String filesDirectory = ho.getControlsContext().getFilesDir().getAbsolutePath();
        nativeHandle = nativeCreate(editData, version, filesDirectory);
        return false;
    }

    @Override
    public int handleRunObject() {
        return nativeHandle == 0 ? 0 : nativeHandle(nativeHandle);
    }

    @Override
    public void destroyRunObject(boolean fast) {
        if (nativeHandle != 0) {
            nativeDestroy(nativeHandle, fast);
            nativeHandle = 0;
        }
    }

    @Override
    public void action(int id, CActExtension act) {
        if (nativeHandle != 0) {
            nativeAction(nativeHandle, id, actionParameters(act, IniPlusPlusAceParams.forAction(id)));
        }
    }

    @Override
    public boolean condition(int id, CCndExtension cnd) {
        return nativeHandle != 0
                && nativeCondition(nativeHandle, id, conditionParameters(cnd, IniPlusPlusAceParams.forCondition(id)));
    }

    @Override
    public CValue expression(int id) {
        if (nativeHandle == 0) {
            return new CValue(0);
        }
        return nativeExpression(nativeHandle, id, expressionParameters(IniPlusPlusAceParams.forExpression(id)));
    }

    private Object[] actionParameters(CActExtension act, byte[] types) {
        Object[] values = new Object[types.length];
        for (int index = 0; index < types.length; ++index) {
            switch (types[index]) {
                case IniPlusPlusAceParams.STRING:
                    values[index] = act.getParamExpString(rh, index);
                    break;
                case IniPlusPlusAceParams.NUMBER:
                    values[index] = Double.valueOf(act.getParamExpDouble(rh, index));
                    break;
                case IniPlusPlusAceParams.INTEGER:
                    values[index] = Integer.valueOf(act.getParamExpression(rh, index));
                    break;
                case IniPlusPlusAceParams.POSITION: {
                    CPositionInfo position = act.getParamPosition(rh, index);
                    values[index] = Integer.valueOf(packPosition(position.x, position.y));
                    break;
                }
                case IniPlusPlusAceParams.FILENAME:
                    values[index] = act.getParamFilename(rh, index);
                    break;
                case IniPlusPlusAceParams.FILENAME2:
                    values[index] = act.getParamFilename2(rh, index);
                    break;
                case IniPlusPlusAceParams.OBJECT:
                default:
                    // Fusion object selectors/custom payloads are not portable through the JNI
                    // bridge. A null placeholder preserves the generated ACE parameter index.
                    values[index] = null;
                    break;
            }
        }
        return values;
    }

    private Object[] conditionParameters(CCndExtension cnd, byte[] types) {
        Object[] values = new Object[types.length];
        for (int index = 0; index < types.length; ++index) {
            switch (types[index]) {
                case IniPlusPlusAceParams.STRING:
                    values[index] = cnd.getParamExpString(rh, index);
                    break;
                case IniPlusPlusAceParams.NUMBER:
                    values[index] = Double.valueOf(cnd.getParamExpDouble(rh, index));
                    break;
                case IniPlusPlusAceParams.INTEGER:
                    values[index] = Integer.valueOf(cnd.getParamExpression(rh, index));
                    break;
                case IniPlusPlusAceParams.POSITION: {
                    CPositionInfo position = cnd.getParamPosition(rh, index);
                    values[index] = Integer.valueOf(packPosition(position.x, position.y));
                    break;
                }
                case IniPlusPlusAceParams.FILENAME:
                    values[index] = cnd.getParamFilename(rh, index);
                    break;
                case IniPlusPlusAceParams.FILENAME2:
                    values[index] = cnd.getParamFilename2(rh, index);
                    break;
                case IniPlusPlusAceParams.OBJECT:
                default:
                    values[index] = null;
                    break;
            }
        }
        return values;
    }

    private Object[] expressionParameters(byte[] types) {
        Object[] values = new Object[types.length];
        for (int index = 0; index < types.length; ++index) {
            CValue value = ho.getExpParam();
            if (value == null) {
                values[index] = null;
                continue;
            }
            switch (types[index]) {
                case IniPlusPlusAceParams.STRING:
                case IniPlusPlusAceParams.FILENAME:
                case IniPlusPlusAceParams.FILENAME2:
                    values[index] = value.getString();
                    break;
                case IniPlusPlusAceParams.INTEGER:
                case IniPlusPlusAceParams.POSITION:
                    values[index] = Integer.valueOf(value.getInt());
                    break;
                case IniPlusPlusAceParams.NUMBER:
                    values[index] = Double.valueOf(value.getDouble());
                    break;
                case IniPlusPlusAceParams.OBJECT:
                default:
                    values[index] = null;
                    break;
            }
        }
        return values;
    }

    private static int packPosition(int x, int y) {
        return ((x & 0xffff) << 16) | (y & 0xffff);
    }

    private static native int nativeGetNumberOfConditions();
    private static native long nativeCreate(byte[] editData, int version, String filesDirectory);
    private static native void nativeDestroy(long handle, boolean fast);
    private static native int nativeHandle(long handle);
    private static native void nativeAction(long handle, int id, Object[] parameters);
    private static native boolean nativeCondition(long handle, int id, Object[] parameters);
    private static native CValue nativeExpression(long handle, int id, Object[] parameters);
}
