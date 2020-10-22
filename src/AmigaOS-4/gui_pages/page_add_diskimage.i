
PAGE_Add, VGroupObject,

	LAYOUT_AddChild, HGroupObject,
		LAYOUT_AddChild, MakeString(ID_PREFS_FILE_GAD),

		LAYOUT_AddChild, MakeImageButton(ID_PREFS_FILE_SELECT_GAD,BAG_POPFILE),
			CHILD_WeightedWidth, 0,
	LayoutEnd,
	CHILD_WeightedHeight, 0,
	CHILD_Label, MakeLabel(ID_PREFS_FILE_SELECT_GAD), 

	LAYOUT_AddChild, MakeCheck(ID_PREFS_READ_ONLY_GAD,0),
	CHILD_Label, MakeLabel(ID_PREFS_READ_ONLY_GAD), 

LayoutEnd,
