PAGE_Add, VGroupObject,
	LAYOUT_AddChild, VGroupObject,
			GFrame(_L(ID_VIRTUAL_SCSI_DEVICES)),

			LAYOUT_AddChild, HGroupObject,
				LAYOUT_AddChild, MakePopFile(ID_PREFS_ID_0_DEVICE_GAD, MAX_STRING_LENGTH, ""),
				CHILD_Label, MakeLabel(ID_PREFS_ID_0_DEVICE_GAD),
				//LAYOUT_AddChild, MakeInteger(ID_PREFS_ID_0_UNIT_GAD, 5),
				LAYOUT_AddChild, MakeInteger_min_max(ID_PREFS_ID_0_UNIT_GAD, 5,0,128),
				CHILD_WeightedWidth, 0,
//				CHILD_Label, MakeLabel(ID_PREFS_SCSI_UNIT_GAD),
			LayoutEnd,
			LAYOUT_AddChild, HGroupObject,
				LAYOUT_AddChild, MakePopFile(ID_PREFS_ID_1_DEVICE_GAD, MAX_STRING_LENGTH, ""),
				CHILD_Label, MakeLabel(ID_PREFS_ID_1_DEVICE_GAD),
				//LAYOUT_AddChild, MakeInteger(ID_PREFS_ID_1_UNIT_GAD, 5),
				LAYOUT_AddChild, MakeInteger_min_max(ID_PREFS_ID_1_UNIT_GAD, 5,0,128),
				CHILD_WeightedWidth, 0,
//				CHILD_Label, MakeLabel(ID_PREFS_SCSI_UNIT_GAD),
			LayoutEnd,
			LAYOUT_AddChild, HGroupObject,
				LAYOUT_AddChild, MakePopFile(ID_PREFS_ID_2_DEVICE_GAD, MAX_STRING_LENGTH, ""),
				CHILD_Label, MakeLabel(ID_PREFS_ID_2_DEVICE_GAD),
				//LAYOUT_AddChild, MakeInteger(ID_PREFS_ID_2_UNIT_GAD, 5),
				LAYOUT_AddChild, MakeInteger_min_max(ID_PREFS_ID_2_UNIT_GAD, 5,0,128),
				CHILD_WeightedWidth, 0,
//				CHILD_Label, MakeLabel(ID_PREFS_SCSI_UNIT_GAD),
			LayoutEnd,
			LAYOUT_AddChild, HGroupObject,
				LAYOUT_AddChild, MakePopFile(ID_PREFS_ID_3_DEVICE_GAD, MAX_STRING_LENGTH, ""),
				CHILD_Label, MakeLabel(ID_PREFS_ID_3_DEVICE_GAD),
				//LAYOUT_AddChild, MakeInteger(ID_PREFS_ID_3_UNIT_GAD, 5),
				LAYOUT_AddChild, MakeInteger_min_max(ID_PREFS_ID_3_UNIT_GAD, 5,0,128),
				CHILD_WeightedWidth, 0,
//				CHILD_Label, MakeLabel(ID_PREFS_SCSI_UNIT_GAD),
			LayoutEnd,
			LAYOUT_AddChild, HGroupObject,
				LAYOUT_AddChild, MakePopFile(ID_PREFS_ID_4_DEVICE_GAD, MAX_STRING_LENGTH, ""),
				CHILD_Label, MakeLabel(ID_PREFS_ID_4_DEVICE_GAD),
				//LAYOUT_AddChild, MakeInteger(ID_PREFS_ID_4_UNIT_GAD, 5),
				LAYOUT_AddChild, MakeInteger_min_max(ID_PREFS_ID_4_UNIT_GAD, 5,0,128),
				CHILD_WeightedWidth, 0,
//				CHILD_Label, MakeLabel(ID_PREFS_SCSI_UNIT_GAD),
			LayoutEnd,
			LAYOUT_AddChild, HGroupObject,
				LAYOUT_AddChild, MakePopFile(ID_PREFS_ID_5_DEVICE_GAD, MAX_STRING_LENGTH, ""),
				CHILD_Label, MakeLabel(ID_PREFS_ID_5_DEVICE_GAD),
				//LAYOUT_AddChild, MakeInteger(ID_PREFS_ID_5_UNIT_GAD, 5),
				LAYOUT_AddChild, MakeInteger_min_max(ID_PREFS_ID_5_UNIT_GAD, 5,0,128),
				CHILD_WeightedWidth, 0,
//				CHILD_Label, MakeLabel(ID_PREFS_SCSI_UNIT_GAD),
			LayoutEnd,
			LAYOUT_AddChild, HGroupObject,
				LAYOUT_AddChild, MakePopFile(ID_PREFS_ID_6_DEVICE_GAD, MAX_STRING_LENGTH, ""),
				CHILD_Label, MakeLabel(ID_PREFS_ID_6_DEVICE_GAD),
				//LAYOUT_AddChild, MakeInteger(ID_PREFS_ID_6_UNIT_GAD, 5),
				LAYOUT_AddChild, MakeInteger_min_max(ID_PREFS_ID_6_UNIT_GAD, 5,0,128),
				CHILD_WeightedWidth, 0,
//				CHILD_Label, MakeLabel(ID_PREFS_SCSI_UNIT_GAD),
			LayoutEnd,
	LayoutEnd,
PageEnd,

