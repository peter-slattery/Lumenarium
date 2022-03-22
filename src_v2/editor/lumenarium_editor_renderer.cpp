
internal void
edr_render(App_State* state)
{
  glMatrixMode(GL_TEXTURE_2D);
  glLoadIdentity();
  
  glClearColor(0.1f, 0.1f, 0.1f, 1);
  
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  
  glDisable(GL_TEXTURE_2D);
  
  glViewport(0, 0, 1600, 900);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}