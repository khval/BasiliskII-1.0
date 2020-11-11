
PAGE_Add, VGroupObject,

	LAYOUT_AddChild, VGroupObject,
		GFrame(_L(ID_HARDWARE)),

		LAYOUT_AddChild, VGroupObject,
			GFrame(_L(ID_CPU)),

			LAYOUT_AddChild, VGroupObject,
				LAYOUT_AddChild, MakeCycle(ID_PREFS_SYSTEM_CPU_GAD, CPUNames),
				CHILD_Label, MakeLabel(ID_PREFS_SYSTEM_CPU_GAD),

				LAYOUT_AddChild, MakeCheck(ID_PREFS_SYSTEM_FPU_GAD, 0 ),
				CHILD_Label, MakeLabel(ID_PREFS_SYSTEM_FPU_GAD),
			LayoutEnd,
		LayoutEnd,

		LAYOUT_AddChild, VGroupObject,
			GFrame(_L(ID_CPU_PRI)),
			LAYOUT_AddChild, MakeInteger_min_max(ID_CPU_ACTIVE_GAD, 0,-128,0),
				CHILD_Label, MakeLabel(ID_CPU_ACTIVE_GAD),
			LAYOUT_AddChild, MakeInteger_min_max(ID_CPU_INACTIVE_GAD, 0,-128,0), 
				CHILD_Label, MakeLabel(ID_CPU_INACTIVE_GAD),
		LayoutEnd,

		LAYOUT_AddChild, VGroupObject,
			GFrame(_L(ID_RAM)),

			LAYOUT_AddChild, MakeCycle(ID_PREFS_SYSTEM_RAM_GAD, RamNames),
			CHILD_Label, MakeLabel(ID_PREFS_SYSTEM_RAM_GAD),
		LayoutEnd,

	LayoutEnd,
	CHILD_WeightedHeight, 0,

	LAYOUT_AddChild, VGroupObject,
		GFrame(_L(ID_SYSTEM)),

		LAYOUT_AddChild, MakeCycle(ID_PREFS_SYSTEM_MODEL_GAD, ModelNames),
		CHILD_Label, MakeLabel(ID_PREFS_SYSTEM_MODEL_GAD),

		LAYOUT_AddChild, HGroupObject,
			LAYOUT_AddChild, MakeImageButton(ID_PREFS_SYSTEM_ROM_SELECT_GAD,BAG_POPFILE),
				CHILD_WeightedWidth, 0,
			LAYOUT_AddChild, MakePopFile(ID_PREFS_SYSTEM_ROM_GAD, 1024, ""),
		LayoutEnd,
		CHILD_Label, MakeLabel(ID_PREFS_SYSTEM_ROM_SELECT_GAD),

	LayoutEnd,
	CHILD_WeightedHeight, 0,
PageEnd,
