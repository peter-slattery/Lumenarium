
typedef u32 Incenter_City_Id;

enum {
  MONTH_jan,
  MONTH_feb,
  MONTH_mar,
  MONTH_apr,
  MONTH_may,
  MONTH_jun,
  MONTH_jul,
  MONTH_aug,
  MONTH_sep,
  MONTH_oct,
  MONTH_nov,
  MONTH_dec,
};
typedef u32 Incenter_Month_Id;

typedef struct Incenter_City_Desc Incenter_City_Desc;
struct Incenter_City_Desc
{
  Incenter_City_Id id;

  r32 lat;
  r32 lon;

  u32 sacn_universe;

  // TODO: Some way to access this cities strip of leds
};

typedef struct Incenter_Data_Row Incenter_Data_Row;
struct Incenter_Data_Row
{
  Incenter_City_Id id;
  u32 year;
  Incenter_Month_Id month;
  r64 prop;
};

// TODO(PS): fix this to be the real universe
#define incenter_secondary_city_universe 128
#include "incenter_gen_cities.h"

// Data
#include "../../run_tree/data/incenter_data/c/question_1.h"
#include "../../run_tree/data/incenter_data/c/question_2.h"
#include "../../run_tree/data/incenter_data/c/question_3.h"
//#include "../../run_tree/data/incenter_data/c/question_4.h"
//#include "../../run_tree/data/incenter_data/c/question_5.h"
#include "../../run_tree/data/incenter_data/c/question_6.h"
#include "../../run_tree/data/incenter_data/c/question_7.h"
#include "../../run_tree/data/incenter_data/c/question_8.h"
#include "../../run_tree/data/incenter_data/c/question_9.h"
#include "../../run_tree/data/incenter_data/c/question_10.h"
#include "../../run_tree/data/incenter_data/c/question_11.h"
#include "../../run_tree/data/incenter_data/c/question_12.h"
#include "../../run_tree/data/incenter_data/c/question_13.h"
#include "../../run_tree/data/incenter_data/c/question_14.h"
#include "../../run_tree/data/incenter_data/c/question_15.h"
#include "../../run_tree/data/incenter_data/c/question_16.h"
#include "../../run_tree/data/incenter_data/c/question_17.h"
#include "../../run_tree/data/incenter_data/c/question_18.h"
#include "../../run_tree/data/incenter_data/c/question_19.h"
#include "../../run_tree/data/incenter_data/c/question_20.h"

typedef struct Incenter_State Incenter_State;

typedef void Incenter_Pattern(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, Incenter_State* state);

typedef u32 Incenter_Scene_Mode;
enum {
  Incenter_SceneMode_Intro,
  Incenter_SceneMode_Input,
  Incenter_SceneMode_Passive,
  Incenter_SceneMode_Count,

  Incenter_SceneMode_TransitioningOut,
  Incenter_SceneMode_TransitioningIn,
};

#define INCENTER_TRANSITION_DURATION 5
#define INCENTER_TRANSITION_SUN_REVEAL_DURATION 3

typedef struct Incenter_Scene Incenter_Scene;
struct Incenter_Scene
{
  char* name;
  Incenter_Pattern* patterns[Incenter_SceneMode_Count];
  Incenter_Data_Row* data;
  u32                data_len;
};

struct Incenter_State
{
  Incenter_Scene*     scenes;
  u32                 scenes_cap;
  u32                 scene_at;
  u32                 scene_next;
  Incenter_Scene_Mode scene_mode;
  r64                 scene_time;
  r64                 transition_time;
};