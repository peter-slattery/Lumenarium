
#define sloth_test_assert(c) if (!(c)) { do{ *((volatile int*)0) = 0xFFFF; }while(0); }

#define sloth_r32_equals(a,b) (fabsf((a) - (b)) < 0.001f)

Sloth_U32
sloth_test_string_len(char* s)
{
  char* at = s;
  while (*at != 0) at++;
  return (Sloth_U32)(at - s);
}

bool
sloth_test_strings_equal(char* a, char* b)
{
  Sloth_U32 a_len = sloth_test_string_len(a);
  Sloth_U32 b_len = sloth_test_string_len(b);
  if (a_len != b_len) return false;
  for (Sloth_U32 i = 0; i < a_len; i++) {
    if (a[i] != b[i]) return false;
  }
  return true;
}

static Sloth_U32 sloth_test_widget_order_count = 0;
void
sloth_test_widget_order(Sloth_Ctx* ctx, Sloth_Widget* widget, Sloth_U8* user_data)
{
  Sloth_ID* ids = (Sloth_ID*)user_data;
  Sloth_ID id_at = ids[sloth_test_widget_order_count++];
  sloth_test_assert(sloth_ids_equal(widget->id, id_at));
}

// Naive string sizing
Sloth_V2
sloth_test_get_text_size(Sloth_Widget* widget)
{
  Sloth_V2 result = {
    .x = widget->text_len * 14,
    .y = 14,
  };
  return result;
}

// The error this is trying to catch takes place across three frames
// Frame 1: all widgets are drawn
// Frame 2: a widget early in teh tree is removed
// Frame 3: a widget late in the tree is removed
//   importantly, this widget comes after a widget that
//   has children. ie. The tree must look like:
//     root
//       <removed on frame 1>
//       widget
//         child
//       <removed on frame 2>
void
sloth_test_multi_frame_removal_frame(Sloth_Ctx* sloth, int num_to_remove)
{
  Sloth_Widget_Desc d = {};
  sloth_frame_prepare(sloth);
  
  sloth_push_widget(sloth, d, "Root"); // root
  {  
    if (num_to_remove < 1) {
      sloth_push_widget(sloth, d, "remove 1");
      sloth_pop_widget(sloth);
    }
    
    sloth_push_widget(sloth, d, "bar_bounds_inner");
    {
      sloth_push_widget(sloth, d, "fg_bar");
      sloth_pop_widget(sloth);
    }
    sloth_pop_widget(sloth);
    
    if (num_to_remove < 2) {
      sloth_push_widget(sloth, d, "remove 2");
      sloth_pop_widget(sloth);
    }
  }
  sloth_pop_widget(sloth);
  
  sloth_frame_advance(sloth);
}
void
sloth_test_multi_frame_removal_frame_b(Sloth_Ctx* sloth, int num_to_remove)
{
  Sloth_Widget_Desc d = {};
  sloth_frame_prepare(sloth);
  
  sloth_push_widget(sloth, d, "Root"); // root
  {  
    if (num_to_remove < 1) {
      sloth_push_widget(sloth, d, "remove 1");
      sloth_pop_widget(sloth);
    }
    
    sloth_push_widget(sloth, d, "a");
    {
      sloth_push_widget(sloth, d, "bar_bounds_inner");
      {
        sloth_push_widget(sloth, d, "fg_bar");
        sloth_pop_widget(sloth);
      }
      sloth_pop_widget(sloth);
      
      if (num_to_remove < 2) {
        sloth_push_widget(sloth, d, "remove 2");
        sloth_pop_widget(sloth);
      }
    }
    sloth_pop_widget(sloth);
  }
  sloth_pop_widget(sloth);
  
  sloth_frame_advance(sloth);
}

void
sloth_test_multi_frame_removal()
{
  Sloth_Ctx sloth = {};
  sloth_test_multi_frame_removal_frame(&sloth, 0);
  sloth_test_multi_frame_removal_frame(&sloth, 1);
  sloth_test_multi_frame_removal_frame(&sloth, 2);
  sloth_ctx_free(&sloth);    
  
  sloth = (Sloth_Ctx){};
  sloth_test_multi_frame_removal_frame_b(&sloth, 0);
  sloth_test_multi_frame_removal_frame_b(&sloth, 1);
  sloth_test_multi_frame_removal_frame_b(&sloth, 2);
  sloth_ctx_free(&sloth);    
}

void
sloth_tests()
{

  sloth_test_assert(sloth_is_pow2(2048));
  sloth_test_assert(!sloth_is_pow2(1920));

  // ID Creation Tests
  char test_id_str[]   = "Test id##53";
  int  test_id_str_len = (sizeof(test_id_str) / sizeof(char)) - 1;

  Sloth_ID_Result id0 = sloth_make_id(test_id_str);
  sloth_test_assert(id0.id.value != 0);
  sloth_test_assert(id0.display_len == test_id_str_len - 4);

  Sloth_ID_Result id1 = sloth_make_id_len(11, "Test id##53");
  sloth_test_assert(id0.id.value == id1.id.value);

  Sloth_ID_Result id2 = sloth_make_id_f("Test id###%d", 53);
  sloth_test_assert(id2.id.value != 0);
  sloth_test_assert(id2.id.value != id0.id.value);
  sloth_test_assert(id2.display_len == 7);

  // Vectors
  Sloth_V2 va = { 25, 32.1f };
  Sloth_V2 vb = { 19, 18.1f };
  
  Sloth_V2 rv0 = sloth_v2_add(va, vb);
  sloth_test_assert(sloth_r32_equals(rv0.x, 44));
  sloth_test_assert(sloth_r32_equals(rv0.y, 50.2f));

  Sloth_V2 rv1 = sloth_v2_sub(va, vb);
  sloth_test_assert(sloth_r32_equals(rv1.x, 6));
  sloth_test_assert(sloth_r32_equals(rv1.y, 14));

  Sloth_V2 rv2 = sloth_v2_mulf(va, 2);
  sloth_test_assert(sloth_r32_equals(rv2.x, 50));
  sloth_test_assert(sloth_r32_equals(rv2.y, 64.2f));

  // Rects
  // baseline rect
  Sloth_Rect rect0 = {
    .value_min = { 0, 0 },
    .value_max = { 100, 100 },
  };
  // overlaps rect0 to right and top
  Sloth_Rect rect1 = {
    .value_min = { 50, 50 },
    .value_max = { 150, 150 },
  };
  // overlaps rect1 to the left and bottom
  Sloth_Rect rect2 = {
    .value_min = { -50, -50 },
    .value_max = { 50, 50 },
  };
  // no overlap with rect0 to the left and bottom
  Sloth_Rect rect3 = {
    .value_min = { -250, -250 },
    .value_max = { -200, -200 }
  };
  // no overlap with rect0 to the right and top
  Sloth_Rect rect4 = {
    .value_min = { 250, 250 },
    .value_max = { 200, 200 }
  };
  // contains rect0
  Sloth_Rect rect5 = {
    .value_min = { -50, -50 },
    .value_max = { 200, 200 }
  };

  Sloth_Rect rr0 = sloth_rect_union(rect0, rect1);
  sloth_test_assert(rr0.value_min.x == 50 && rr0.value_min.y == 50);
  sloth_test_assert(rr0.value_max.x == 100 && rr0.value_max.y == 100);

  Sloth_Rect rr1 = sloth_rect_union(rect0, rect2);
  sloth_test_assert(rr1.value_min.x == 0 && rr1.value_min.y == 0);
  sloth_test_assert(rr1.value_max.x == 50 && rr1.value_max.y == 50);

  Sloth_Rect rr2 = sloth_rect_union(rect0, rect3);
  sloth_test_assert(rr2.value_min.x == 0 && rr2.value_min.y == 0);
  sloth_test_assert(rr2.value_max.x == 0 && rr2.value_max.y == 0);

  Sloth_Rect rr3 = sloth_rect_union(rect0, rect4);
  sloth_test_assert(rr3.value_min.x == 0 && rr3.value_min.y == 0);
  sloth_test_assert(rr3.value_max.x == 0 && rr3.value_max.y == 0);

  Sloth_Rect rr4 = sloth_rect_union(rect0, rect5);
  sloth_test_assert(rr4.value_min.x == 0 && rr4.value_min.y == 0);
  sloth_test_assert(rr4.value_max.x == 100 && rr4.value_max.y == 100);

  // contained by rect0
  Sloth_V2 rectp0 = { 25, 25 };
  // not contained by rect0 to the right and top
  Sloth_V2 rectp1 = { 150, 150 };
  // not contained by rect0 to the left and bottom
  Sloth_V2 rectp2 = { -25, -25 };
  
  sloth_test_assert(sloth_rect_contains(rect0, rectp0));
  sloth_test_assert(!sloth_rect_contains(rect0, rectp1));
  sloth_test_assert(!sloth_rect_contains(rect0, rectp2));

  // Hashtable Tests
  {
    Sloth_Hashtable table = {};
    sloth_hashtable_add(&table, 256, (Sloth_U8*)1);
    sloth_hashtable_add(&table, 394, (Sloth_U8*)2);
    sloth_hashtable_add(&table, 81932, (Sloth_U8*)3);
    
    // this should force chaining
    sloth_hashtable_add(&table, table.cap + 256, (Sloth_U8*)4);

    Sloth_U64 v0 = (Sloth_U64)sloth_hashtable_get(&table, 256);
    sloth_test_assert(v0 == 1);
    Sloth_U64 v1 = (Sloth_U64)sloth_hashtable_get(&table, 394);
    sloth_test_assert(v1 == 2);
    Sloth_U64 v2 = (Sloth_U64)sloth_hashtable_get(&table, 81932);
    sloth_test_assert(v2 == 3);
    Sloth_U64 v3 = (Sloth_U64)sloth_hashtable_get(&table, table.cap + 256);
    sloth_test_assert(v3 == 4);

    // getting a value that's not present
    Sloth_U64 vi = (Sloth_U64)sloth_hashtable_get(&table, 3333);
    sloth_test_assert(vi == 0);

    Sloth_Bool r0 = sloth_hashtable_rem(&table, 256);
    sloth_test_assert(r0);
    v0 = (Sloth_U64)sloth_hashtable_get(&table, 256);
    sloth_test_assert(v0 == 0);
  }

  { // Arena Tests

    Sloth_Arena arena = {};
    Sloth_U32* array_0 = sloth_arena_push_array(&arena, Sloth_U32, 32);
    for (Sloth_U32 i = 0; i < 32; i++) array_0[i] = i;
    sloth_test_assert(array_0 != 0);

    Sloth_Arena_Loc old_at = sloth_arena_at(&arena);
    sloth_test_assert(old_at.bucket_at == sizeof(Sloth_U32) * 32);
        
    Sloth_U32* array_1 = sloth_arena_push_array(&arena, Sloth_U32, 32);
    for (Sloth_U32 i = 0; i < 32; i++) array_1[i] = (i + 32);
    sloth_test_assert(array_1 >= (array_0 + 32));
    sloth_test_assert(array_1 != 0);
    sloth_test_assert(array_0[31] == 31);
    sloth_test_assert(array_1[0] == 32);

    // testing memory reuse after popping
    sloth_arena_pop(&arena, old_at);
    // test that in debug mode, popped memory is cleared
    // NOTE: that if we aren't in debug mode, sloth_test_assert evaluates to
    // nothing, so the test won't run
    for (Sloth_U32 i = 0; i < 32; i++) sloth_test_assert(array_1[i] == 0);

    Sloth_U32* array_1b = sloth_arena_push_array(&arena, Sloth_U32, 32);
    sloth_test_assert(array_1 == array_1b);

    // testing memory reuse after clearing
    sloth_arena_clear(&arena);
    Sloth_U32* array_0b = sloth_arena_push_array(&arena, Sloth_U32, 32);
    sloth_test_assert(array_0 == array_0b);

    sloth_arena_free(&arena);
    sloth_test_assert(!arena.buckets);
    sloth_test_assert(!arena.buckets_len);
    sloth_test_assert(!arena.buckets_cap);
    sloth_test_assert(!arena.bucket_cap);
    sloth_test_assert(!arena.curr_bucket_len);
  }
  
  { // Gamma correction
    Sloth_R32 r_in = 0.2f;
    Sloth_R32 g_in = 0.5f;
    Sloth_R32 b_in = 0.9f;
    Sloth_R32 a_in = 0.1f;
    Sloth_U32 color = (((Sloth_U32)(r_in * 255) << 24) |
                       ((Sloth_U32)(g_in * 255) << 16) |
                       ((Sloth_U32)(b_in * 255) <<  8) |
                       ((Sloth_U32)(a_in * 255)));
    
    // gamma = 1, no change
    Sloth_U32 color_out0 = sloth_color_apply_gamma(color, 1);
    sloth_assert(color_out0 == color);
    
    // gamma = 2.2, verify changes
    Sloth_U32 color_out1 = sloth_color_apply_gamma(color, 2.2f);
    Sloth_R32 r = (Sloth_R32)((color_out1 >> 24) & 0xFF) / 255.0f;
    Sloth_R32 g = (Sloth_R32)((color_out1 >> 16) & 0xFF) / 255.0f;
    Sloth_R32 b = (Sloth_R32)((color_out1 >>  8) & 0xFF) / 255.0f;
    Sloth_R32 a = (Sloth_R32)((color_out1      ) & 0xFF) / 255.0f;
    
    Sloth_R32 delta_r = fabsf(r - powf(r_in, 2.2f));
    Sloth_R32 delta_g = fabsf(g - powf(g_in, 2.2f));
    Sloth_R32 delta_b = fabsf(b - powf(b_in, 2.2f));
    Sloth_R32 delta_a = fabsf(a - powf(a_in, 2.2f));
    
    sloth_assert(delta_r < 0.01f);
    sloth_assert(delta_g < 0.01f);
    sloth_assert(delta_b < 0.01f);
    sloth_assert(delta_a < 0.01f);
  }
  
  { // Atlas Tests
    Sloth_U32 test_icon[] = {
      0xFFFFFFFF, 0x000000FF, 0xFFFFFFFF, 0x000000FF,
      0x000000FF, 0xFFFFFFFF, 0x000000FF, 0xFFFFFFFF,
      0xFFFFFFFF, 0x000000FF, 0xFFFFFFFF, 0x000000FF,
      0x000000FF, 0xFFFFFFFF, 0x000000FF, 0xFFFFFFFF,
    };

    Sloth_Glyph_Atlas atlas = {};
    sloth_glyph_atlas_resize(&atlas, 32);
    
    Sloth_Glyph_Desc gd0 = {
      .family = 1,
      .id = 25,
      .data = (Sloth_U8*)test_icon,
      .width = 4,
      .height = 4,
      .stride = 4,
      .format = Sloth_GlyphData_RGBA8,
    };
    Sloth_Glyph_ID id_0 = sloth_glyph_atlas_register(&atlas, gd0);
    Sloth_U32 last_glyph = atlas.last_glyph;
    sloth_test_assert(atlas.glyphs_table.used == 1);

    // testing adding the same glyph a second time. 
    Sloth_Glyph_ID id_01 = sloth_glyph_atlas_register(&atlas, gd0);
    sloth_test_assert(id_01.value == id_0.value);
    sloth_test_assert(atlas.last_glyph == last_glyph);
    sloth_test_assert(atlas.glyphs_table.used == 1); // no sprite was added
    
    Sloth_Glyph_Desc gd2 = gd0;
    gd2.id = 26;
    Sloth_Glyph_ID id_2 = sloth_glyph_atlas_register(&atlas, gd2);
    sloth_test_assert(id_2.value != 0);
    
    Sloth_Glyph_Desc gd3 = gd0;
    gd3.id = 27;
    Sloth_Glyph_ID id_3 = sloth_glyph_atlas_register(&atlas, gd3);
    sloth_test_assert(id_3.value != 0);
    
    Sloth_Glyph_Desc gd4 = gd0;
    gd4.id = 28;
    Sloth_Glyph_ID id_4 = sloth_glyph_atlas_register(&atlas, gd4);
    sloth_test_assert(id_4.value != 0);
    sloth_test_assert(id_4.family == gd0.family);
    sloth_test_assert(id_4.id[0] == 28 && id_4.id[1] == 0 && id_4.id[2] == 0);
    
    Sloth_Glyph_Desc gd5 = gd0;
    gd5.id = 29; 
    Sloth_Glyph_ID id_5 = sloth_glyph_atlas_register(&atlas, gd5);
    sloth_test_assert(id_5.value != 0);
    sloth_test_assert(id_5.family == 1);
    sloth_test_assert(id_5.id[0] == 29 && id_5.id[1] == 0 && id_5.id[2] == 0);

    sloth_glyph_atlas_free(&atlas);
    
    // Glyph ID Tests
    Sloth_Glyph_ID g_id = sloth_make_glyph_id(24, 'G');
    Sloth_Glyph_ID newline_id = sloth_make_glyph_id(32, '\n');
    Sloth_Glyph_ID space_id = sloth_make_glyph_id(127, ' ');
    sloth_test_assert(sloth_glyph_id_matches_charcode(g_id, 'G'));
    sloth_test_assert(sloth_glyph_id_matches_charcode(newline_id, '\n'));
    sloth_test_assert(sloth_glyph_id_matches_charcode(space_id, ' '));
  }
  
  { // Sloth_Size tests
    
    // see @Maintenance tag in Sloth_Size_Box if this fails
    Sloth_Size_Box b = {
      .left = sloth_size_pixels(0),
      .right = sloth_size_pixels(5),
      .top = sloth_size_pixels(10),
      .bottom = sloth_size_pixels(15),
    };
    
    // testing to make sure left corresponds to E[Axis_X].min
    // and so on
    sloth_test_assert(b.E[Sloth_Axis_X].min.value == 0);
    sloth_test_assert(b.E[Sloth_Axis_X].max.value == 5);
    sloth_test_assert(b.E[Sloth_Axis_Y].min.value == 10);
    sloth_test_assert(b.E[Sloth_Axis_Y].max.value == 15);
  }
  
  // Widget Tree Construction
  {
    Sloth_Ctx sloth = {
      .per_frame_memory.name = "pfm",
      .scratch.name = "scratch",
    };

    Sloth_Widget_Desc d = {}; // these tests don't depend on the desc at all
    Sloth_ID ids0_preorder[] = {
      sloth_make_id_f("Root").id, 
      sloth_make_id_f("W1").id, 
      sloth_make_id_f("W11").id, 
      sloth_make_id_f("W12").id, 
      sloth_make_id_f("W2").id, 
      sloth_make_id_f("W3").id, 
      sloth_make_id_f("W31").id
    };
    Sloth_ID ids0_postorder[] = {
      sloth_make_id_f("W11").id,
      sloth_make_id_f("W12").id,
      sloth_make_id_f("W1").id,
      sloth_make_id_f("W2").id,
      sloth_make_id_f("W31").id,
      sloth_make_id_f("W3").id,
      sloth_make_id_f("Root").id,
    };
    printf("Frame 1\n");
    sloth_frame_prepare(&sloth);
    sloth_push_widget(&sloth, d, "Root"); // root
      sloth_push_widget(&sloth, d, "W1");
        sloth_push_widget(&sloth, d, "W11"); sloth_pop_widget(&sloth);
        sloth_push_widget(&sloth, d, "W12"); sloth_pop_widget(&sloth);
      sloth_pop_widget(&sloth);
      sloth_push_widget(&sloth, d, "W2"); sloth_pop_widget(&sloth);
      sloth_push_widget(&sloth, d, "W3"); 
        sloth_push_widget(&sloth, d, "W31"); sloth_pop_widget(&sloth);
      sloth_pop_widget(&sloth);
    sloth_pop_widget(&sloth); // root - won't pop

    // walking the tree
    sloth_test_assert(sloth.widget_tree_root != 0);
    sloth_test_assert(sloth.widget_tree_depth_max == 3);
    sloth_test_widget_order_count = 0; // reset test
    sloth_tree_walk_preorder(&sloth, sloth_test_widget_order, (Sloth_U8*)&ids0_preorder);
    sloth_test_widget_order_count = 0; // reset test
    sloth_tree_walk_postorder(&sloth, sloth_test_widget_order, (Sloth_U8*)&ids0_postorder);
    //sloth_widget_tree_print(&sloth);

    sloth_frame_advance(&sloth);
    printf("Frame 2\n");
    sloth_frame_prepare(&sloth);

    // Same Frame as above
    sloth_push_widget(&sloth, d, "Root"); // root
      sloth_push_widget(&sloth, d, "W1"); 
        sloth_push_widget(&sloth, d, "W11"); sloth_pop_widget(&sloth);
        sloth_push_widget(&sloth, d, "W12"); sloth_pop_widget(&sloth);
      sloth_pop_widget(&sloth);
      sloth_push_widget(&sloth, d, "W2"); sloth_pop_widget(&sloth);
      sloth_push_widget(&sloth, d, "W3"); 
        sloth_push_widget(&sloth, d, "W31"); sloth_pop_widget(&sloth);
      sloth_pop_widget(&sloth);
    sloth_pop_widget(&sloth); // root - won't pop

    // walking the tree
    sloth_test_assert(sloth.widget_tree_root != 0);
    sloth_test_widget_order_count = 0; // reset test
    sloth_tree_walk_preorder(&sloth, sloth_test_widget_order, (Sloth_U8*)&ids0_preorder);

    sloth_frame_advance(&sloth);
    sloth_frame_prepare(&sloth);

    // Different frame from above
    Sloth_ID ids1[] = {
      sloth_make_id_f("Root").id, 
      sloth_make_id_f("W1").id, 
      sloth_make_id_f("W11").id, 
      sloth_make_id_f("W13").id, 
      sloth_make_id_f("W14").id, 
      sloth_make_id_f("W12").id, 
      sloth_make_id_f("W2").id, 
      sloth_make_id_f("W21").id, 
      sloth_make_id_f("W3").id,
    };
    sloth_push_widget(&sloth, d, "Root"); // root
      sloth_push_widget(&sloth, d, "W1"); 
        sloth_push_widget(&sloth, d, "W11"); sloth_pop_widget(&sloth);
        sloth_push_widget(&sloth, d, "W13"); sloth_pop_widget(&sloth);
        sloth_push_widget(&sloth, d, "W14"); sloth_pop_widget(&sloth);
        sloth_push_widget(&sloth, d, "W12"); sloth_pop_widget(&sloth);
      sloth_pop_widget(&sloth);
      sloth_push_widget(&sloth, d, "W2"); 
        sloth_push_widget(&sloth, d, "W21"); sloth_pop_widget(&sloth);
      sloth_pop_widget(&sloth);
      sloth_push_widget(&sloth, d, "W3"); 
        // old child should get freed
      sloth_pop_widget(&sloth);
    sloth_pop_widget(&sloth); // root - won't pop

    sloth_test_widget_order_count = 0; // reset test
    sloth_tree_walk_preorder(&sloth, sloth_test_widget_order, (Sloth_U8*)&ids1);    

    sloth_ctx_free(&sloth);
  }
  
  
  // Widget Tree - Removing Expected Next Sibling
  sloth_test_multi_frame_removal();
  
  // Widget Sizing
  {
    Sloth_Ctx sloth = {
    };

    sloth_frame_prepare(&sloth);

    Sloth_Widget_Desc ele_desc;
    Sloth_Widget_Desc root_desc = {
      .layout = {
        .width = sloth_size_pixels(800),
        .height = sloth_size_pixels(900),
        //.margin.top = sloth_size_pixels(32),
        .direction = Sloth_LayoutDirection_TopDown,
      },
      .style.color_bg = 0x333333FF,
    };
    sloth_push_widget(&sloth, root_desc, "root");
      ele_desc = (Sloth_Widget_Desc){
        .layout = {
          .width = sloth_size_pixels(850),
          .height = sloth_size_pixels(200),
        },
        .style.color_bg = 0xFFFFFFFF,
      };
      sloth_push_widget(&sloth, ele_desc, "ele0"); sloth_pop_widget(&sloth);
      ele_desc.style.color_bg = 0xFF00FFFF;
      sloth_push_widget(&sloth, ele_desc, "ele1"); sloth_pop_widget(&sloth);
    sloth_pop_widget(&sloth);

    printf("==/==\n");
    sloth_frame_advance(&sloth);
    sloth_frame_prepare(&sloth);

    Sloth_Widget* root = sloth.widget_tree_next_child;
    sloth_test_assert(root->cached->offset.x == 0 && root->cached->offset.y == 0);
    sloth_test_assert(root->cached->dim.x == 800 && root->cached->dim.y == 900);

    Sloth_Widget* ele0 = root->child_first;
    sloth_test_assert(ele0->cached->offset.x == 0 && ele0->cached->offset.y == 0);
    sloth_test_assert(ele0->cached->dim.x == 800 && ele0->cached->dim.y == 200);

    Sloth_Widget* ele1 = ele0->sibling_next;
    sloth_test_assert(ele1->cached->offset.x == 0 && ele1->cached->offset.y == 200);
    sloth_test_assert(ele1->cached->dim.x == 800 && ele1->cached->dim.y == 200);
    
    sloth_ctx_free(&sloth);
  }

  return;
}

bool
sloth_test_button_f(Sloth_Ctx* sloth, char* fmt, ...)
{
  Sloth_Widget_Desc desc = {
    .layout = {
      .width = sloth_size_text_content(),
      .height = sloth_size_text_content(),
      .margin = {
        .left = sloth_size_pixels(12),
        .right = sloth_size_pixels(12),
        .top = sloth_size_pixels(0),
        .bottom = sloth_size_pixels(8),
      },
    },
    .style = {
      .color_bg = 0x333333FF,
      .color_text = 0xFFFFFFFF,
      .color_outline = 0xFFFFFFFF,
      .outline_thickness = 1,
      .text_style = Sloth_TextStyle_Align_Center,
    },
  };
  va_list args; va_start(args, fmt);
  Sloth_Widget_Result r = sloth_push_widget_v(sloth, desc, fmt, args);
  va_end(args);
  sloth_pop_widget(sloth);
  
  if (r.clicked) {
    r.widget->style.color_bg = 0xFFFFFFFF;
    r.widget->style.color_text = 0x333333FF;
  }
  
  return r.clicked;
}

Sloth_R32
sloth_test_slider_f(Sloth_Ctx* sloth, Sloth_R32 min, Sloth_R32 max, Sloth_R32 v, char* fmt, ...)
{
  Sloth_Widget_Desc desc = {
    .layout = {
      .width = sloth_size_pixels(128),
      .height = sloth_size_pixels(32),
    },
    .style = {
      .color_bg = 0x333333FF,
      .color_outline = 0xFFFFFFFF,
      .outline_thickness = 1,
    },
    .input.flags = Sloth_WidgetInput_Draggable
  };
  
  va_list args; va_start(args, fmt);
  Sloth_Widget_Result r = sloth_push_widget_v(sloth, desc, fmt, args);
  va_end(args);
  
  // background text
  Sloth_Widget_Desc bg_text_desc = {
    .layout = {
      .width = sloth_size_percent_parent(1),
      .height = sloth_size_percent_parent(1),
      .position = {
        .kind= Sloth_LayoutPosition_FixedInParent,
        .left = sloth_size_pixels(0),
        .top = sloth_size_pixels(0),
      },
      .margin.left = sloth_size_pixels(8),
    },
    .style = {
      .color_text = 0xFFFFFFFF,
      .text_style = Sloth_TextStyle_NoWrapText
    },
    .input.flags = Sloth_WidgetInput_DoNotCaptureMouse,
  };
  sloth_push_widget_f(sloth, bg_text_desc, "%f###%d_bg", v, r.widget->id.value);
  sloth_pop_widget(sloth);
  
  // slider bar
  Sloth_R32 pct = (v - min) / (max - min);
  Sloth_Widget_Desc slider_desc = {
    .layout = {
      .width = sloth_size_percent_parent(pct),
      .height = sloth_size_percent_parent(1),
      .margin.left = sloth_size_pixels(8),
    },
    .style = {
      .color_bg = 0x00FFFFFF,
      .color_text = 0x000000FF,
      .text_style = Sloth_TextStyle_NoWrapText
    },
    .input.flags = Sloth_WidgetInput_DoNotCaptureMouse
  };
  Sloth_Widget_Result rs = sloth_push_widget_f(sloth, slider_desc, "%f###%d_slider", v, r.widget->id.value);
  //printf("%d\n", r.widget->id.value);
  sloth_pop_widget(sloth);
  
  sloth_pop_widget(sloth);
  
  
  if (r.clicked) {
    rs.widget->style.color_bg = 0xFFFFFFFF;
  }
  
  Sloth_R32 result = v;
  if (r.held) {
    Sloth_R32 width = sloth_rect_dim(r.widget->cached->bounds).x;
    Sloth_R32 init_pct = (sloth->mouse_down_pos.x - r.widget->cached->bounds.value_min.x) / width;
    Sloth_R32 dx = r.drag_offset_pixels.x;
    Sloth_R32 px = dx / width;
    result = ((px + init_pct) * (max - min)) + min;
    result = Sloth_Max(min, Sloth_Min(max, result));
  }
  return result;
}

