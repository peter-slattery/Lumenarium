/* date = August 2nd 2022 5:47 am */

#ifndef INCENTER_SCENES_H
#define INCENTER_SCENES_H

typedef u8 Incenter_Scene_ID;
enum {
  Incenter_Scene_Invalid = 0,
  
  Incenter_Scene_AnyoneHome = 1,
  Incenter_Scene_WelcomeHome = 2,
  Incenter_Scene_Question_FeltIsolated = 3,
  Incenter_Scene_Question_FeltFearAnxiety = 4,
  Incenter_Scene_Question_FeltPowerless = 5,
  Incenter_Scene_Question_LostAccessToResources = 6,
  Incenter_Scene_Question_LostLovedOne = 7,
  Incenter_Scene_Question_BegunToHeal = 8,
  Incenter_Scene_OnPlayaResources = 9,
  Incenter_Scene_Question_RelationshipCommunitySupport = 10,
  Incenter_Scene_Question_ConnectionFriendsFamily = 11,
  Incenter_Scene_Question_ValueConnections = 12,
  Incenter_Scene_Question_FindHappiness = 13,
  Incenter_Scene_Question_FeltExcludedIdentity = 14,
  Incenter_Scene_Question_RepresentedByLeadership = 15,
  Incenter_Scene_Question_LearningOpenMinded = 16,
  Incenter_Scene_Question_ProtectOurEarth = 17,
  Incenter_Scene_Question_BelieveScienceRenewableTech = 18,
  Incenter_Scene_Question_StriveMoreEcoFriendly = 19,
  Incenter_Scene_Question_ActionToHelpPlanet = 20,
  Incenter_Scene_Question_HowYouFaceChallenges = 21,
  Incenter_Scene_Question_InspiredToHelpOthers = 22,
  Incenter_Scene_Question_PracticeChangeHopeFor = 23,
  Incenter_Scene_Question_PracticeRadicalInclusion = 24,
  Incenter_Scene_Question_CommunityFeelBelong = 25,
  Incenter_Scene_Credits = 26,
  
  Incenter_Scene_Count,
};

global Incenter_Scene incenter_scene_descs[Incenter_Scene_Count];

internal void incenter_scene_descs_init();

#endif //INCENTER_SCENES_H
