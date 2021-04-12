

PAGE_Add, VGroupObject,

	LAYOUT_AddChild, VGroupObject,
		GFrame(_L(ID_MAC_VOLUMES)),

		StartMember, obj[ID_MAC_VOLUMES] = (Object *) ListBrowserObject,
			GA_ID, ID_MAC_VOLUMES,
			GA_RelVerify, TRUE,
			LISTBROWSER_Labels, &list_files,
			LISTBROWSER_ColumnInfo, volumes_ci,
			LISTBROWSER_ColumnTitles, TRUE,
			//LISTBROWSER_MultiSelect, FALSE,
			//LISTBROWSER_Separators, FALSE,
			LISTBROWSER_ShowSelected, TRUE,
			//LISTBROWSER_Editable, FALSE,
		EndMember,
//--------------

		LAYOUT_AddChild, HGroupObject,
			LAYOUT_AddChild, MakeButton(ID_PREFS_ADD_BOOTDISK_GAD),
			LAYOUT_AddChild, MakeButton(ID_PREFS_ADD_GAD),
			LAYOUT_AddChild, MakeButton(ID_PREFS_CREATE_GAD),
			LAYOUT_AddChild, MakeButton(ID_PREFS_EDIT_GAD),
			LAYOUT_AddChild, MakeButton(ID_PREFS_REMOVE_GAD),
		LayoutEnd,
		CHILD_WeightedHeight, 0,
	LayoutEnd,

	LAYOUT_AddChild, VGroupObject,
		GFrame(_L(ID_CDROM)) ,

		LAYOUT_AddChild, MakeCycle(ID_PREFS_CD_DEVICE_GAD, device_names),
		CHILD_Label, MakeLabel(ID_PREFS_CD_DEVICE_GAD), 

		//LAYOUT_AddChild, MakeInteger(ID_PREFS_CD_UNIT_GAD, 0),
		LAYOUT_AddChild, MakeInteger_min_max(ID_PREFS_CD_UNIT_GAD, 0,0,128),
		CHILD_Label, MakeLabel(ID_PREFS_CD_UNIT_GAD), 

		//LAYOUT_AddChild, MakeCheck(ID_PREFS_CD_BOOT_GAD, 0),
		//CHILD_Label, MakeLabel(ID_PREFS_CD_BOOT_GAD),
		LAYOUT_AddChild, MakeCheckR(ID_PREFS_CD_BOOT_GAD, FALSE),
		CHILD_Label, MakeLabelEmpty(),

		//LAYOUT_AddChild, MakeCheck(ID_PREFS_CD_DISABLE_DRIVER_GAD, 0),
		//CHILD_Label, MakeLabel(ID_PREFS_CD_DISABLE_DRIVER_GAD),
		LAYOUT_AddChild, MakeCheckR(ID_PREFS_CD_DISABLE_DRIVER_GAD, FALSE),
		CHILD_Label, MakeLabelEmpty(),
	LayoutEnd,
	CHILD_WeightedHeight, 0,

	LAYOUT_AddChild, HGroupObject,
		LAYOUT_AddChild, MakeImageButton(ID_PREFS_AMIGAOS4_ROOT_SELECT_GAD,BAG_POPDRAWER),
			CHILD_WeightedWidth, 0,
		LAYOUT_AddChild, MakeString(ID_PREFS_AMIGAOS4_ROOT_GAD),
	LayoutEnd,
	CHILD_WeightedHeight, 0,
	CHILD_Label, MakeLabel(ID_PREFS_AMIGAOS4_ROOT_SELECT_GAD),

//--------------

PageEnd,
