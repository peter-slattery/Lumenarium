
internal void
incenter_scene_descs_init()
{
  incenter_scene_descs[Incenter_Scene_AnyoneHome] = (Incenter_Scene){
    .name = "AnyoneHome",
    .patterns = {
      [Incenter_SceneMode_Intro] = pattern_anybody_home,
      [Incenter_SceneMode_Passive] = pattern_anybody_home,
    },
    .kind = Incenter_SceneKind_Information,
  };
  
  incenter_scene_descs[Incenter_Scene_WelcomeHome] = (Incenter_Scene){
    .name = "WelcomeHome",
    .patterns = {
      [Incenter_SceneMode_Intro] = pattern_sun_passive,
      [Incenter_SceneMode_Passive] = pattern_sun_passive,
    },
    .kind = Incenter_SceneKind_Information,
  };
  
  incenter_scene_descs[Incenter_Scene_Question_FeltIsolated] = (Incenter_Scene){
    .name = "FeltIsolated",
    .patterns = {
      [Incenter_SceneMode_Intro] = pattern_felt_isolated_intro,
      [Incenter_SceneMode_Passive] = pattern_felt_isolated_passive,
    },
    .data = question_1_data,
    .data_len = question_1_len,
    .kind = Incenter_SceneKind_YesOrNo,
  };
  
  incenter_scene_descs[Incenter_Scene_Question_FeltFearAnxiety] = (Incenter_Scene){
    .name = "FeltFearAnxiety",
    .patterns = {
      [Incenter_SceneMode_Intro] = pattern_felt_isolated_intro,
      [Incenter_SceneMode_Passive] = pattern_felt_isolated_passive,
    },
    .data = question_2_data,
    .data_len = question_2_len,
    .kind = Incenter_SceneKind_ThreeOption,
  };
  
  incenter_scene_descs[Incenter_Scene_Question_FeltPowerless] = (Incenter_Scene){
    .name = "FeltPowerless",
    .patterns = {
      [Incenter_SceneMode_Intro] = pattern_felt_isolated_intro,
      [Incenter_SceneMode_Passive] = pattern_felt_isolated_passive,
    },
    .data = question_5_data,
    .data_len = question_5_len,
    .kind = Incenter_SceneKind_YesOrNo,
  };
  
  incenter_scene_descs[Incenter_Scene_Question_LostAccessToResources] = (Incenter_Scene){
    .name = "LostAccessToResources",
    .patterns = {
      [Incenter_SceneMode_Intro] = pattern_bar_chart,
      [Incenter_SceneMode_Passive] = pattern_bar_chart,
    },
    .data = question_3_data,
    .data_len = question_3_len,
    .kind = Incenter_SceneKind_YesOrNo,
  };
  
  incenter_scene_descs[Incenter_Scene_Question_LostLovedOne] = (Incenter_Scene){
    .name = "LostLovedOne",
    .patterns = {
      [Incenter_SceneMode_Intro] = pattern_bar_chart_over_time,
      [Incenter_SceneMode_Passive] = pattern_bar_chart_over_time,
    },
    .data = question_4_data,
    .data_len = question_4_len,
    .kind = Incenter_SceneKind_YesOrNo,
  };
  
  // TODO: We need an animation that communicates healing / connection
  // grow out from the person's answer into the entire planet
  incenter_scene_descs[Incenter_Scene_Question_BegunToHeal] = (Incenter_Scene){
    .name = "BegunToHeal",
    .patterns = {
      [Incenter_SceneMode_Intro] = pattern_bar_chart_bubbly_intro,
      [Incenter_SceneMode_Passive] = pattern_bar_chart_bubbly_passive,
    },
    // no data    .data = question_6_data,
    // no data   .data_len = question_6_len,
    .kind = Incenter_SceneKind_SlidingScale,
  };
  
  incenter_scene_descs[Incenter_Scene_OnPlayaResources] = (Incenter_Scene){
    .name = "OnPlayaResources",
    .patterns = {
      [Incenter_SceneMode_Intro] = pattern_bar_chart,
      [Incenter_SceneMode_Passive] = pattern_bar_chart,
    },
    .kind = Incenter_SceneKind_Information,
  };
  
  incenter_scene_descs[Incenter_Scene_Question_RelationshipCommunitySupport] = (Incenter_Scene){
    .name = "RelationshipCommunitySupport",
    .patterns = {
      [Incenter_SceneMode_Intro] = pattern_bar_chart_with_connections,
      [Incenter_SceneMode_Passive] = pattern_bar_chart_with_connections,
    },
    .data = question_7_data,
    .data_len = question_7_len,
    .kind = Incenter_SceneKind_SlidingScale,
  };
  
  incenter_scene_descs[Incenter_Scene_Question_ConnectionFriendsFamily] = (Incenter_Scene){
    .name = "ConnectionFriendsFamily",
    .patterns = {
      [Incenter_SceneMode_Intro] = pattern_bar_chart_with_connections,
      [Incenter_SceneMode_Passive] = pattern_bar_chart_with_connections,
    },
    .data = question_8_data,
    .data_len = question_8_len,
    .kind = Incenter_SceneKind_SlidingScale,
  };
  
  incenter_scene_descs[Incenter_Scene_Question_ValueConnections] = (Incenter_Scene){
    .name = "ValueConnections",
    .patterns = {
#if 0
      [Incenter_SceneMode_Intro] = pattern_bar_chart_with_connections,
      [Incenter_SceneMode_Passive] = pattern_bar_chart_with_connections,
#endif
      [Incenter_SceneMode_Intro] = pattern_bar_chart_random_fill,
      [Incenter_SceneMode_Passive] = pattern_bar_chart_random_fill,
    },
    .data = question_9_data,
    .data_len = question_9_len,
    .kind = Incenter_SceneKind_SlidingScale,
  };
  
  incenter_scene_descs[Incenter_Scene_Question_FindHappiness] = (Incenter_Scene){
    .name = "FindHappiness",
    .patterns = {
      [Incenter_SceneMode_Intro] = pattern_bar_chart,
      [Incenter_SceneMode_Passive] = pattern_bar_chart,
    },
    .data = question_10_data,
    .data_len = question_10_len,
    .kind = Incenter_SceneKind_YesOrNo,
  };
  
  incenter_scene_descs[Incenter_Scene_Question_FeltExcludedIdentity] = (Incenter_Scene){
    .name = "FeltExcludedIdentity",
    .patterns = {
      [Incenter_SceneMode_Intro] = pattern_bar_chart,
      [Incenter_SceneMode_Passive] = pattern_bar_chart,
    },
    .data = question_13_data,
    .data_len = question_13_len,
    .kind = Incenter_SceneKind_YesOrNo,
  };
  
  incenter_scene_descs[Incenter_Scene_Question_RepresentedByLeadership] = (Incenter_Scene){
    .name = "RepresentedByLeadership",
    .patterns = {
      [Incenter_SceneMode_Intro] = pattern_bar_chart,
      [Incenter_SceneMode_Passive] = pattern_bar_chart,
    },
    .data = question_14_data,
    .data_len = question_14_len,
    .kind = Incenter_SceneKind_SlidingScale,
  };
  
  incenter_scene_descs[Incenter_Scene_Question_LearningOpenMinded] = (Incenter_Scene){
    .name = "LearningOpenMinded",
    .patterns = {
      [Incenter_SceneMode_Intro] = pattern_bar_chart,
      [Incenter_SceneMode_Passive] = pattern_bar_chart,
    },
    .data = question_12_data,
    .data_len = question_12_len,
    .kind = Incenter_SceneKind_SlidingScale,
  };
  
  incenter_scene_descs[Incenter_Scene_Question_ProtectOurEarth] = (Incenter_Scene){
    .name = "ProtectOurEarth",
    .patterns = {
      [Incenter_SceneMode_Intro] = pattern_bar_chart,
      [Incenter_SceneMode_Passive] = pattern_bar_chart,
    },
    .data = question_18_data,
    .data_len = question_18_len,
    .kind = Incenter_SceneKind_YesOrNo,
  };
  
  incenter_scene_descs[Incenter_Scene_Question_BelieveScienceRenewableTech] = (Incenter_Scene){
    .name = "BelieveScienceRenewableTech",
    .patterns = {
      [Incenter_SceneMode_Intro] = pattern_bar_chart_random_fill,
      [Incenter_SceneMode_Passive] = pattern_bar_chart_random_fill,
    },
    .data = question_19_data,
    .data_len = question_19_len,
    .kind = Incenter_SceneKind_YesOrNo,
  };
  
  incenter_scene_descs[Incenter_Scene_Question_StriveMoreEcoFriendly] = (Incenter_Scene){
    .name = "StriveMoreEcoFriendly",
    .patterns = {
      [Incenter_SceneMode_Intro] = pattern_bar_chart,
      [Incenter_SceneMode_Passive] = pattern_bar_chart,
    },
    //    .data = question_22_data,
    //    .data_len = question_22_len,
    .kind = Incenter_SceneKind_YesOrNo,
  };
  
  
  incenter_scene_descs[Incenter_Scene_Question_ActionToHelpPlanet] = (Incenter_Scene){
    .name = "ActionToHelpPlanet",
    .patterns = {
      [Incenter_SceneMode_Intro] = pattern_bar_chart,
      [Incenter_SceneMode_Passive] = pattern_bar_chart,
    },
    .data = question_20_data,
    .data_len = question_20_len,
    .kind = Incenter_SceneKind_YesOrNo,
  };
  
  incenter_scene_descs[Incenter_Scene_Question_HowYouFaceChallenges] = (Incenter_Scene){
    .name = "HowYouFaceChallenges",
    .patterns = {
      [Incenter_SceneMode_Intro] = pattern_bar_chart,
      [Incenter_SceneMode_Passive] = pattern_bar_chart,
    },
    .data = question_6_data,
    .data_len = question_6_len,
    .kind = Incenter_SceneKind_YesOrNo,
  };
  
  incenter_scene_descs[Incenter_Scene_Question_InspiredToHelpOthers] = (Incenter_Scene){
    .name = "InspiredToHelpOthers",
    .patterns = {
      [Incenter_SceneMode_Intro] = pattern_bar_chart,
      [Incenter_SceneMode_Passive] = pattern_bar_chart,
    },
    .data = question_11_data,
    .data_len = question_11_len,
    .kind = Incenter_SceneKind_YesOrNo,
  };
  
  incenter_scene_descs[Incenter_Scene_Question_PracticeChangeHopeFor] = (Incenter_Scene){
    .name = "PracticeChangeHopeFor",
    .patterns = {
      [Incenter_SceneMode_Intro] = pattern_bar_chart,
      [Incenter_SceneMode_Passive] = pattern_bar_chart,
    },
    .data = question_17_data,
    .data_len = question_17_len,
    .kind = Incenter_SceneKind_YesOrNo,
  };
  
  incenter_scene_descs[Incenter_Scene_Question_PracticeRadicalInclusion] = (Incenter_Scene){
    .name = "PracticeRadicalInclusion",
    .patterns = {
      [Incenter_SceneMode_Intro] = pattern_bar_chart,
      [Incenter_SceneMode_Passive] = pattern_bar_chart,
    },
    .data = question_16_data,
    .data_len = question_16_len,
    .kind = Incenter_SceneKind_YesOrNo,
  };
  
  incenter_scene_descs[Incenter_Scene_Question_CommunityFeelBelong] = (Incenter_Scene){
    .name = "CommunityFeelBelong",
    .patterns = {
      [Incenter_SceneMode_Intro] = pattern_bar_chart,
      [Incenter_SceneMode_Passive] = pattern_bar_chart,
    },
    .data = question_15_data,
    .data_len = question_15_len,
    .kind = Incenter_SceneKind_YesOrNo,
  };
  
  incenter_scene_descs[Incenter_Scene_Credits] = (Incenter_Scene){
    .name = "Credits",
    .patterns = {
      [Incenter_SceneMode_Intro] = pattern_bar_chart,
      [Incenter_SceneMode_Passive] = pattern_bar_chart,      
    },
    .kind = Incenter_SceneKind_Information,
  };
}
