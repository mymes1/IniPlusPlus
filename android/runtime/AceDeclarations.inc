// This file is included at global scope (see IniPlusPlusExtension.cc).
namespace ipp_android
{
	inline constexpr int number_of_actions     = 84;
	inline constexpr int number_of_conditions  = 20;
	inline constexpr int number_of_expressions = 41;
}

// ---- action ACEs (84) ------------------------------------------------------------------
auto FUSION_API actionSetCurrentGroup(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 0
auto FUSION_API actionSetValueG(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 1
auto FUSION_API actionSetStringG(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 2
auto FUSION_API actionSetStringMD5G(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 3
auto FUSION_API actionSaveObjectG(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 4
auto FUSION_API actionLoadObjectG(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 5
auto FUSION_API actionSavePositionG(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 6
auto FUSION_API actionSaveGlobalValuesG(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 7
auto FUSION_API actionLoadGlobalValuesG(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 8
auto FUSION_API actionDeleteGroupG(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 9
auto FUSION_API actionDeleteItemG(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 10
auto FUSION_API actionRenameGroupG(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 11
auto FUSION_API actionRenameItemG(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 12
auto FUSION_API actionMoveItemToGroupG(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 13
auto FUSION_API actionSetValue(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 14
auto FUSION_API actionSetString(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 15
auto FUSION_API actionSetStringMD5(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 16
auto FUSION_API actionSaveObject(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 17
auto FUSION_API actionLoadObject(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 18
auto FUSION_API actionSavePosition(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 19
auto FUSION_API actionSaveAltValues(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 20
auto FUSION_API actionLoadAltValues(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 21
auto FUSION_API actionRenameGroup(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 22
auto FUSION_API actionRenameItem(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 23
auto FUSION_API actionMoveItem(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 24
auto FUSION_API actionMoveItemToGroup(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 25
auto FUSION_API actionCopyGroup(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 26
auto FUSION_API actionCopyItem(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 27
auto FUSION_API actionDeleteGroup(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 28
auto FUSION_API actionDeleteItem(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 29
auto FUSION_API actionDeleteItemInAllGroups(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 30
auto FUSION_API actionDeleteFromPattern(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 31
auto FUSION_API actionClearAll(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 32
auto FUSION_API actionPerformSearch(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 33
auto FUSION_API actionPerformMultiplesSearch(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 34
auto FUSION_API actionClearResults(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 35
auto FUSION_API actionCreateSubINI(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 36
auto FUSION_API actionCreateSubINIObject(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 37
auto FUSION_API actionMergeFile(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 38
auto FUSION_API actionMergeGroupFile(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 39
auto FUSION_API actionMerge(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 40
auto FUSION_API actionMergeGroupObject(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 41
auto FUSION_API actionNew(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 42
auto FUSION_API actionLoad(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 43
auto FUSION_API actionSave(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 44
auto FUSION_API actionSaveAs(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 45
auto FUSION_API actionBackupTo(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 46
auto FUSION_API actionClose(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 47
auto FUSION_API actionLoadFromString(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 48
auto FUSION_API actionSetAutoSave(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 49
auto FUSION_API actionSetCompression(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 50
auto FUSION_API actionSetEncryption(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 51
auto FUSION_API actionSetReadOnly(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 52
auto FUSION_API actionSetCaseSensitive(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 53
auto FUSION_API actionSetEscapeChars(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 54
auto FUSION_API actionNeverQuoteStrings(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 55
auto FUSION_API actionSetRepeatModes(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 56
auto FUSION_API actionSetNewLineChar(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 57
auto FUSION_API actionSetDefaultDirectory(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 58
auto FUSION_API actionCompressFile(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 59
auto FUSION_API actionDeCompressFile(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 60
auto FUSION_API actionOpenDialog(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 61
auto FUSION_API actionAddRepeatedItem(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 62
auto FUSION_API actionCloseDialog(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 63
auto FUSION_API actionRefreshDialog(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 64
auto FUSION_API actionExportCVS(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 65
auto FUSION_API actionImportCVS(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 66
auto FUSION_API actionToChart(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 67
auto FUSION_API actionFindSubGroups(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 68
auto FUSION_API actionEnableSubGroups(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 69
auto FUSION_API actionSwitchGlobalObject(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 70
auto FUSION_API actionSSS(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 71
auto FUSION_API actionSetItemArray(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 72
auto FUSION_API actionLoadFromArray(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 73
auto FUSION_API actionSaveToArray(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 74
auto FUSION_API actionFromChart(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 75
auto FUSION_API actionSaveChartSettings(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 76
auto FUSION_API actionLoadChartSettings(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 77
auto FUSION_API actionLoadChangeFile(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 78
auto FUSION_API actionUndo(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 79
auto FUSION_API actionRedo(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 80
auto FUSION_API actionClearUndoStack(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 81
auto FUSION_API actionAddNewUndoBlock(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 82
auto FUSION_API actionSetManualMode(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::action_return_t; // 83

// ---- condition ACEs (20) ------------------------------------------------------------------
auto FUSION_API conditionHasGroupG(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::condition_return_t; // 0
auto FUSION_API conditionHasItemG(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::condition_return_t; // 1
auto FUSION_API conditionCompareMD5G(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::condition_return_t; // 2
auto FUSION_API conditionHasGroup(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::condition_return_t; // 3
auto FUSION_API conditionHasItem(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::condition_return_t; // 4
auto FUSION_API conditionCompareMD5(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::condition_return_t; // 5
auto FUSION_API conditionWildcatMatch(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::condition_return_t; // 6
auto FUSION_API conditionEvent(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::condition_return_t; // 7
auto FUSION_API conditionEvent(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::condition_return_t; // 8
auto FUSION_API conditionEvent(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::condition_return_t; // 9
auto FUSION_API conditionEvent(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::condition_return_t; // 10
auto FUSION_API conditionEvent(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::condition_return_t; // 11
auto FUSION_API conditionEvent(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::condition_return_t; // 12
auto FUSION_API conditionEvent(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::condition_return_t; // 13
auto FUSION_API conditionEvent(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::condition_return_t; // 14
auto FUSION_API conditionEvent(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::condition_return_t; // 15
auto FUSION_API conditionEvent(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::condition_return_t; // 16
auto FUSION_API conditionEvent(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::condition_return_t; // 17
auto FUSION_API conditionHasUndo(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::condition_return_t; // 18
auto FUSION_API conditionHasRedo(::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const) noexcept -> ::fusion::condition_return_t; // 19

// ---- expression ACEs (41) ------------------------------------------------------------------
auto FUSION_API expressionGetItemValueG(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 0
auto FUSION_API expressionGetItemStringG(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 1
auto FUSION_API expressionGetXG(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 2
auto FUSION_API expressionGetYG(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 3
auto FUSION_API expressionNthItemG(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 4
auto FUSION_API expressionNthItemValueG(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 5
auto FUSION_API expressionNthItemStringG(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 6
auto FUSION_API expressionItemCountG(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 7
auto FUSION_API expressionGetItemValue(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 8
auto FUSION_API expressionGetItemString(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 9
auto FUSION_API expressionGetX(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 10
auto FUSION_API expressionGetY(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 11
auto FUSION_API expressionNthGroup(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 12
auto FUSION_API expressionNthItem(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 13
auto FUSION_API expressionNthItemString(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 14
auto FUSION_API expressionNthItemValue(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 15
auto FUSION_API expressionGroupCount(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 16
auto FUSION_API expressionItemCount(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 17
auto FUSION_API expressionTotalItems(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 18
auto FUSION_API expressionSearchResultCounts(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 19
auto FUSION_API expressionGetNthSearchResultGroup(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 20
auto FUSION_API expressionGetNthSearchResultItem(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 21
auto FUSION_API expressionGetNthSearchResultItemValue(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 22
auto FUSION_API expressionGetNthSearchResultItemString(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 23
auto FUSION_API expressionGetNthSearchResultPath(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 24
auto FUSION_API expressionWriteStream(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 25
auto FUSION_API expressionHashString(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 26
auto FUSION_API expressionEncryptString(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 27
auto FUSION_API expressionEscapeString(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 28
auto FUSION_API expressionUnEscapeString(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 29
auto FUSION_API expressionInnerProduct(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 30
auto FUSION_API expressionInnerProductStr(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 31
auto FUSION_API expressionNthSortedName(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 32
auto FUSION_API expressionNthSortedValue(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 33
auto FUSION_API expressionGetCSV(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 34
auto FUSION_API expressionNthOverallItem(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 35
auto FUSION_API expressionUniqueItemCount(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 36
auto FUSION_API expressionGetItemArray(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 37
auto FUSION_API expressionCurGroup(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 38
auto FUSION_API expressionCurGroupString(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 39
auto FUSION_API expressionFname(::RunData* const, ::fusion::e_params_t const) noexcept -> ::fusion::expression_return_t; // 40
