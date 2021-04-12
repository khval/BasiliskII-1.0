
PAGE_Add, VGroupObject,

		LAYOUT_AddChild, MakeCycle(ID_PREFS_DEVICE_GAD, device_names),
		CHILD_MinWidth, 200,
		CHILD_Label, MakeLabel(ID_PREFS_DEVICE_GAD),

		LAYOUT_AddChild, MakeInteger(ID_PREFS_UNIT_GAD, 8),
		CHILD_Label, MakeLabel(ID_PREFS_UNIT_GAD),

		LAYOUT_AddChild, MakeString(ID_PREFS_PARTITION_NAME_GAD),
		CHILD_Label, MakeLabel(ID_PREFS_PARTITION_NAME_GAD),

LayoutEnd,
