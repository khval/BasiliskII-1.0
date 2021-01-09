PAGE_Add, VGroupObject,

	LAYOUT_AddChild, VGroupObject,
		GFrame (_L(ID_ETHERNET)),

		LAYOUT_AddChild, HGroupObject,
			LAYOUT_AddChild, MakeImageButton(ID_PREFS_ETHERNET_DEVICE_SELECT_GAD,BAG_POPFILE),
				CHILD_WeightedWidth, 0,
			LAYOUT_AddChild, MakeString(ID_PREFS_ETHERNET_DEVICE_GAD),
		LayoutEnd,
		CHILD_Label, MakeLabel(ID_PREFS_ETHERNET_DEVICE_SELECT_GAD),

		LAYOUT_AddChild, MakeInteger(ID_PREFS_ETHERNET_UNIT_GAD, 5),
		CHILD_Label, MakeLabel(ID_PREFS_ETHERNET_UNIT_GAD),

		LAYOUT_AddChild, MakeCheck(ID_PREFS_ETHERNET_MONITOR_GAD, 0 ),
		CHILD_Label, MakeLabel(ID_PREFS_ETHERNET_MONITOR_GAD),

	LayoutEnd,
	CHILD_WeightedHeight, 0,

	LAYOUT_AddChild, VGroupObject,
		GFrame(_L(ID_SOUND)),
		LAYOUT_AddChild, MakeCheck(ID_PREFS_DISABLE_SOUND, 0 ),
		CHILD_Label, MakeLabel(ID_PREFS_DISABLE_SOUND),
	LayoutEnd,
	CHILD_WeightedHeight, 0,

	LAYOUT_AddChild, VGroupObject,
		GFrame(_L(ID_MODEM)),

		LAYOUT_AddChild, MakePopFile(ID_PREFS_MODEM_DEVICE_GAD, MAX_STRING_LENGTH, ""),
		CHILD_Label, MakeLabel(ID_PREFS_MODEM_DEVICE_GAD),
		
		LAYOUT_AddChild, HGroupObject,
			LAYOUT_AddChild, MakeInteger(ID_PREFS_MODEM_UNIT_GAD, 5),
			LAYOUT_AddChild, MakeCheck(ID_PREFS_MODEM_PARALLEL_GAD, 0),
			CHILD_Label, MakeLabel(ID_PREFS_MODEM_PARALLEL_GAD),
		LayoutEnd,
		CHILD_Label, MakeLabel(ID_PREFS_MODEM_UNIT_GAD),

	LayoutEnd,
	CHILD_WeightedHeight, 0,

	LAYOUT_AddChild, VGroupObject,
		GFrame(_L(ID_PRINTER)),
		LAYOUT_AddChild, MakePopFile(ID_PREFS_PRINTER_DEVICE_GAD, MAX_STRING_LENGTH, ""),
		CHILD_Label, MakeLabel(ID_PREFS_PRINTER_DEVICE_GAD),

		LAYOUT_AddChild, HGroupObject,
			LAYOUT_AddChild, MakeInteger(ID_PREFS_PRINTER_UNIT_GAD, 0),
			LAYOUT_AddChild, MakeCheck(ID_PREFS_PRINTER_PARALLEL_GAD, 0),
			CHILD_Label, MakeLabel(ID_PREFS_PRINTER_PARALLEL_GAD),
		LayoutEnd,
		CHILD_Label, MakeLabel(ID_PREFS_PRINTER_UNIT_GAD),

	LayoutEnd,
	CHILD_WeightedHeight, 0,

PageEnd,
