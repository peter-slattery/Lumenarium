
#define INCENTER_METER 1.0f
#define INCENTER_FOOT 0.3048f
#define INCENTER_METERS(count) (count) * INCENTER_METER
#define INCENTER_FEET(count) (count) * INCENTER_FOOT
#define INCENTER_PER_METER(count) INCENTER_METER / (r32)(count)

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
#include "../../run_tree/data/incenter_data/c/question_4.h"
#include "../../run_tree/data/incenter_data/c/question_5.h"
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
#include "../../run_tree/data/incenter_data/c/question_21.h"

typedef struct Incenter_State Incenter_State;

typedef void Incenter_Pattern(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, Incenter_State* state);

typedef u32 Incenter_Scene_Mode;
enum {
  Incenter_SceneMode_Intro,
  Incenter_SceneMode_Passive,
  Incenter_SceneMode_Count,
  
  Incenter_SceneMode_Input,
  Incenter_SceneMode_TransitioningOut,
  Incenter_SceneMode_TransitioningIn,
};

#define INCENTER_TRANSITION_DURATION 3
#define INCENTER_TRANSITION_SUN_REVEAL_DURATION 2

typedef u8 Incenter_Scene_Kind;
enum {
  Incenter_SceneKind_Information,
  Incenter_SceneKind_YesOrNo,
  Incenter_SceneKind_ThreeOption,
  Incenter_SceneKind_SlidingScale,
  Incenter_SceneKind_Count,
};

typedef struct Incenter_Scene Incenter_Scene;
struct Incenter_Scene
{
  char* name;
  Incenter_Pattern* patterns[Incenter_SceneMode_Count];
  Incenter_Data_Row* data;
  u32                data_len;
  Incenter_Scene_Kind kind;
};

// INLH is abbrev for INcenter Live data Header
#define LIVE_DATA_HEADER_MAGIC_NUMBER "INLH"
// INLB is abbrev for INcenter Live data Bucket
#define LIVE_DATA_BUCKET_MAGIC_NUMBER "INLB"

typedef struct Live_Answers_File_Header Live_Answers_File_Header;
struct Live_Answers_File_Header
{
  u8 magic[4];
  u32 buckets_count;
  u32 answers_total_count;
};

typedef struct Live_Answers_File_Bucket Live_Answers_File_Bucket;
struct Live_Answers_File_Bucket
{
  u8 magic[4];
  
  // the value this bucket represents.
  union {
    u32 answer_u32;
    r32 answer_r32;
  };
  
  // The number of responses that fit within this bucket
  u32 count;
};

typedef struct Live_Answers_File Live_Answers_File;
struct Live_Answers_File
{
  String path;
  Live_Answers_File_Header* header;
  Live_Answers_File_Bucket* buckets;
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
  
  bool running;
  Thread_Handle interface_thread;
  Socket_Handle interface_socket;
  
  // Ring buffer of interface messages. All 
#define INTERFACE_MESSAGES_CAP 8
#define INTERFACE_MESSAGE_SIZE 512
  Data interface_messages[INTERFACE_MESSAGES_CAP];
  u32  interface_messages_write_next;
  u32  interface_messages_read_next;  
  
  // User Input
  r32 input_pct;
  u32 input_option;
  b8  input_advance;
};

internal v4
incenter_latlng_to_cartesian(r32 lat, r32 lng, r32 radius)
{
  r32 theta = (lat / 180.0f) * r32_pi;
  r32 phi   = (lng / 180.0f) * r32_pi;
  
  // spherical to cartesian conversion
  v4 result = {
    radius * sinf(phi) * cosf(theta),
    radius * sinf(phi) * sinf(theta),
    radius * cosf(phi),
    1
  };
  
  return result;
}
