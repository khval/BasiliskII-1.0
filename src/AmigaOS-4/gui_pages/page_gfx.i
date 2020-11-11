
PAGE_Add, VGroupObject,

	LAYOUT_AddChild, VGroupObject,
		GFrame(_L(ID_GRAPHICS)),

		LAYOUT_AddChild, MakeCycle(ID_PREFS_GFX_MODE_GAD, ModeNames),
		CHILD_Label, MakeLabel(ID_PREFS_GFX_MODE_GAD),

		LAYOUT_AddChild, HGroupObject,
			LAYOUT_AddChild, MakeInteger(ID_PREFS_GFX_WIDTH_GAD, 8),

			LAYOUT_AddChild, MakeInteger(ID_PREFS_GFX_HEIGHT_GAD, 8),
			CHILD_Label, MakeLabel(ID_PREFS_GFX_HEIGHT_GAD),
		LayoutEnd,
		CHILD_Label, MakeLabel(ID_PREFS_GFX_WIDTH_GAD),

		LAYOUT_AddChild, HGroupObject,
			LAYOUT_AddChild, MakeImageButton(ID_PREFS_GFX_MODE_ID_SELECT_GAD,BAG_POPSCREEN),
				CHILD_WeightedWidth, 0,

			LAYOUT_AddChild, MakeHexString(ID_PREFS_GFX_MODE_ID_GAD), 

		LayoutEnd,
		CHILD_Label, MakeLabel(ID_PREFS_GFX_MODE_ID_SELECT_GAD),

		LAYOUT_AddChild, MakeCycle(ID_PREFS_GFX_WINDOW_DEPTH_GAD, window_depth_names),
		CHILD_Label, MakeLabel(ID_PREFS_GFX_WINDOW_DEPTH_GAD), 

		LAYOUT_AddChild, MakeCycle(ID_PREFS_GFX_RENDER_METHOD_GAD, window_render_method_names),
		CHILD_Label, MakeLabel(ID_PREFS_GFX_RENDER_METHOD_GAD), 

		LAYOUT_AddChild, MakeCheck(ID_PREFS_GFX_LOCK_GAD, 0),
		CHILD_Label, MakeLabel(ID_PREFS_GFX_LOCK_GAD),

//		LAYOUT_AddChild, MakeCheck(ID_PREFS_GFX_FSCREEN_32B_GAD, 0),
//		CHILD_Label, MakeLabel(ID_PREFS_GFX_FSCREEN_32B_GAD),

		LAYOUT_AddChild, HGroupObject,
			LAYOUT_AddChild, MakeInteger_min_max(ID_PREFS_GFX_FRAMESKIP_GAD, 0,0,20),
			LAYOUT_AddChild, MakeInteger_min_max(ID_PREFS_GFX_LINESKIP_GAD, 0,0,50), 
				CHILD_Label, MakeLabel(ID_PREFS_GFX_LINESKIP_GAD),
		LayoutEnd,
		CHILD_Label, MakeLabel(ID_PREFS_GFX_FRAMESKIP_GAD),

	LayoutEnd,
	CHILD_WeightedHeight, 0,

	LAYOUT_AddChild, VGroupObject,
		GFrame(_L(ID_SOUND)),
		LAYOUT_AddChild, MakeCheck(ID_PREFS_DISABLE_SOUND, 0 ),
		CHILD_Label, MakeLabel(ID_PREFS_DISABLE_SOUND),
	LayoutEnd,
	CHILD_WeightedHeight, 0,


PageEnd,
