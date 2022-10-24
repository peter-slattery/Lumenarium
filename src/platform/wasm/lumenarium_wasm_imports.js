var lumenarium_wasm_module = null;
var lumenarium_wasm_instance = null;

var WASM_PAGE_SIZE = 65536;

function wasm_mem_get_u8_arr(inst, ptr, size)
{
  let view = new Uint8Array(inst.exports.memory.buffer, ptr, size);
  return view;
}

function wasm_read_string(inst, ptr, len)
{
  let view = wasm_mem_get_u8_arr(inst, ptr, len);
  let string = '';
  for (let i = 0; i < len; i++)
  {
    string += String.fromCharCode(view[i]);
  }
  return string;
}

function wasm_write_bytes(inst, src, ptr, len)
{
  let view = wasm_mem_get_u8_arr(inst, ptr, len);
  for (let i = 0; i < len; i++) view[i] = src[i];
}

function wasm_get_proc(inst, proc_ptr)
{
  let result = inst.exports.__indirect_function_table.get(proc_ptr);
  return result;
}

function fract (v) { return v % 1; }

function u32_to_byte_array_32 (v) 
{
  let result = [0, 0, 0, 0];
  result[0] = (v & 0xff);
  result[1] = (((v - result[0]) >> 8 ) & 0xff);
  result[2] = (((v - result[1]) >> 16) & 0xff);
  result[3] = (((v - result[2]) >> 24) & 0xff);
  return result;
}

function byte_array_32_to_u32 (arr)
{
  // NOTE(PS): the '>>>' operators in this function deal with the fact
  // that bit shift operators convert numbers to s32's. The >>> just
  // converts them back to u32s
  let r0 =  ((arr[0] & 0xff) << 0 );
  let r1 =  ((arr[1] & 0xff) << 8 );
  let r2 =  ((arr[2] & 0xff) << 16);
  let r3 = (((arr[3] & 0xff) << 24) >>> 0);
  let result = (r0 | r1 | r2 | r3) >>> 0;
  return result;
}

function put_u32 (ptr, value)
{
  let src = u32_to_byte_array_32(value);
  wasm_write_bytes(lumenarium_wasm_instance, src, ptr, 4);
}

var lumenarium_wasm_imports = {
  
  memset: (dst, size, value) => {
    let view_dst = wasm_mem_get_u8_arr(lumenarium_wasm_instance, dst, size);
    for (let i = 0; i < size; i++)
    {
      view_dst[i] = value;
    }
  },
  
  memcpy: (dst, src, size) => {
    let view_dst = wasm_mem_get_u8_arr(lumenarium_wasm_instance, dst, size);
    let view_src = wasm_mem_get_u8_arr(lumenarium_wasm_instance, src, size);
    for (let i = 0; i < size; i++)
    {
      view_dst[i] = view_src[i];
    }
  },
  
  wasm_assert_always: (file, file_len, line) => { 
    let file_str = wasm_read_string(lumenarium_wasm_instance, file, file_len);
    console.assert(false, "At: " + file_str + "::" + line); 
  },
  
  wasm_get_memory_size: () => {
    return instance.exports.memory.buffer.byteLength;
  },
  
  wasm_mem_grow: (new_size) => {
    let new_size_ = new_size >>> 0;
    let pages = new_size_ / WASM_PAGE_SIZE;
    let pages_rem = fract(pages);
    if (pages_rem > 0) pages = Math.floor(pages) + 1;
    let size_before = lumenarium_wasm_instance.exports.memory.buffer.byteLength;
    let old_page_count = lumenarium_wasm_instance.exports.memory.grow(pages);
    
    console.log("mem_grow\n", 
                "req size: ", new_size_, "\n",
                "old size: ", (old_page_count * WASM_PAGE_SIZE), "\n",
                "old size: ", size_before, "\n",
                "grew by:  ", (pages * WASM_PAGE_SIZE), "\n", 
                "new size: ", lumenarium_wasm_instance.exports.memory.buffer.byteLength, "");
  },
  
  malloc: (size) => {
    
  },
  
  free: (base) => {
    
  },
  
  sin:   Math.sin,
  sinf:  Math.sin,
  cos:   Math.cos,  
  cosf:  Math.cos,  
  tan:   Math.tan,
  tanf:  Math.tan,  
  asin:  Math.asin,  
  asinf: Math.asin,  
  acos:  Math.acos,
  acosf: Math.acos,
  atan:  Math.atan,
  atanf: Math.atan,  
  pow:   Math.pow,  
  powf:  Math.pow,  
  fmodf: (f,d) => { return f % d; },
  strlen: (ptr) => {
    let len = 0;
    let len_checked = 0;
    let len_to_check = 256;
    let found_end = false;
    while (true)
    {
      let string = wasm_mem_get_u8_arr(lumenarium_wasm_instance, ptr, len_checked);
      for (let i = len_checked; i < len_to_check; i++)
      {
        if (string[i] == 0) 
        {
          len = i;
          break;
        }
      }
      len_checked *= 2;
    }
    return len_checked;
  },
  
  wasm_platform_file_async_work_on_job: (path, path_len, data, data_size, read, write) => {
    
  },
  
  wasm_performance_now: () => {
    return performance.now();
  },
  
  wasm_sleep: (milliseconds) => {
    let start = Date.now();
    for (let at = Date.now(); (at - start) < milliseconds; at = Date.now()) {}
  },
  
  wasm_fetch: async (file_path, file_path_len, dest, dest_size) => {
    let path = wasm_read_string(lumenarium_wasm_instance, file_path, file_path_len);
    fetch(path)
      .then(async (res) => {
              // TODO(PS): success checking
              let reader = res.body.getReader();
              let read_res = { done: false };
              
              let view = wasm_mem_get_u8_arr(lumenarium_wasm_instance, dest, dest_size);
              let last_write = 0;
              while (!read_res.done)
              {
                read_res = await reader.read();
                if (read_res.done) break;
                
                let len = read_res.value.length;
                let write_end = last_write + len;
                for (let i = last_write; i < write_end; i++) 
                {
                  view[i] = read_res.value[i - last_write];
                }
                last_write = write_end + 1;
              }
            });
    return 0;
  },
  
  wasm_request_animation_frame: (cb) => {
    let cb_proc = wasm_get_proc(lumenarium_wasm_instance, cb);
    window.requestAnimationFrame(cb_proc);
  },
  
  print: (str_base, len) => {
    let string = wasm_read_string(lumenarium_wasm_instance, str_base, len);
    console.log(string);
  },
  
  wasm_get_canvas_dim: (w_ptr, h_ptr) => {
    const canvas = document.querySelector("#gl_canvas");
    
    let w_view = wasm_mem_get_u8_arr(lumenarium_wasm_instance, w_ptr, 4);
    let w = canvas.width;
    let wb = u32_to_byte_array_32(w);
    for (let i = 0; i < 4; i++) w_view[i] = wb[i];
    
    let h_view = wasm_mem_get_u8_arr(lumenarium_wasm_instance, h_ptr, 4);
    let h = canvas.height;
    let hb = u32_to_byte_array_32(h);
    for (let i = 0; i < 4; i++) h_view[i] = hb[i];
  },
};

///////////////////////////////////////
// Web GL Imports

let gl = null;
let gl_error = false;

function glErrorReport(outer_args) { 
  const err = gl.getError();
  if (err == gl.NO_ERROR) return;
  
  gl_error = true;
  let msg = "";
  switch (err) {
    case gl.NO_ERROR:                      { msg = "NO_ERROR"; } break;
    case gl.INVALID_ENUM:                  { msg = "INVALID_ENUM"; } break;
    case gl.INVALID_VALUE:                 { msg = "INVALID_VALUE"; } break;
    case gl.INVALID_OPERATION:             { msg = "INVALID_OPERATION"; } break;
    case gl.INVALID_FRAMEBUFFER_OPERATION: { msg = "INVALID_FRAMEBUFFER_OPERATION"; } break;
    case gl.OUT_OF_MEMORY:                 { msg = "OUT_OF_MEMORY"; } break;
    case gl.CONTEXT_LOST_WEBGL:            { msg = "CONTEXT_LOST_WEBGL"; } break;
    default:                               { msg = "Uknown error"; } break;
  }
  console.error(`WebGL Error: ${msg} ${err}`, outer_args);
}

// NOTE(PS): it seems like its not enough to set 
// the values of imports to gl.function
//   ie. imports.glClearColor = gl.clearColor
// instead we need to wrap them for some reason. 
// Not sure why
function glClearColor (r, g, b, a) { return gl.clearColor(r,g,b,a); }
function glEnable(v) { 
  const r = gl.enable(v); 
  glErrorReport(arguments);
  return r;
}
function glDisable(v) { 
  const r = gl.disable(v); 
  glErrorReport(arguments);
  return r;
}
function glBlendFunc(a,b) { 
  const r = gl.blendFunc(a,b); 
  glErrorReport(arguments);
  return r;
}
function glViewport(xmin, ymin, xmax, ymax) { return gl.viewport(xmin,ymin,xmax,ymax); }
function glDepthFunc(v) { 
  const r = gl.depthFunc(v); 
  glErrorReport(arguments);
  return r;
}
function glClear(mask) { 
  const r = gl.clear(mask); 
  glErrorReport(arguments);
  return r;
}

let glBuffers = [];
let glShaders = [];
let glPrograms = [];
let glTextures = [];
function gl_get_managed_resource(arr, id) {
  if (id == 0) return null;
  return arr[id - 1];
}
function gl_get_buffer(id) { return gl_get_managed_resource(glBuffers, id); }
function gl_get_shader(id) { return gl_get_managed_resource(glShaders, id); }
function gl_get_program(id) { return gl_get_managed_resource(glPrograms, id); }
function gl_get_texture(id) { return gl_get_managed_resource(glTextures, id); }

function glCreateBuffer() { 
  let buffer = gl.createBuffer(); 
  glErrorReport(arguments);
  let new_len = glBuffers.push(buffer);
  return new_len;
}

function glBindBuffer(buffer_kind, buffer_id) 
{
  const r = gl.bindBuffer(buffer_kind, gl_get_buffer(buffer_id));
  glErrorReport(arguments);
  return r;
}

function glBufferData(target, size, ptr, usage)
{
  let data = wasm_mem_get_u8_arr(lumenarium_wasm_instance, ptr, size);
  const r = gl.bufferData(target, data, usage);
  glErrorReport(arguments);
  return r;
}

function glBufferSubData(target, offset, size, ptr)
{
  let data = wasm_mem_get_u8_arr(lumenarium_wasm_instance, ptr, size);
  const r = gl.bufferSubData(target, offset, data, 0, size);
  glErrorReport(arguments);
  return r;
}

function glCreateShader(kind)
{
  let shader = gl.createShader(kind);
  glErrorReport(arguments);
  let new_len = glShaders.push(shader);
  return new_len;
}

function glShaderSource(shader_id, shader_code, shader_code_len)
{
  let str = wasm_read_string(lumenarium_wasm_instance, shader_code, shader_code_len);
  const r = gl.shaderSource(gl_get_shader(shader_id), str);
  glErrorReport(arguments);
  return r;
}

function glCompileShader(shader_id)
{
  let s = gl_get_shader(shader_id);
  let r = gl.compileShader(s);
  glErrorReport(arguments);
  let m = gl.getShaderInfoLog(s);
  glErrorReport(arguments);
  if (m.length > 0)
  {
    console.error("glCompileShader: \n\n" + m);
  }
}

function glCreateProgram()
{
  let prog = gl.createProgram();
  glErrorReport(arguments);
  let new_len = glPrograms.push(prog);
  return new_len;
}

function glAttachShader(program, shader)
{
  let s = gl_get_shader(shader);
  let p = gl_get_program(program);
  const r = gl.attachShader(p, s);
  glErrorReport(arguments);
  return r;
}

function glLinkProgram(program)
{
  let p = gl_get_program(program);
  gl.linkProgram(p);
  if (!gl.getProgramParameter(p, gl.LINK_STATUS)) {
    var info = gl.getProgramInfoLog(p);
    console.error("Failed to compile WebGL program. \n\n"+info);
  }
}

function glUseProgram(program)
{
  let p = gl_get_program(program);
  const r = gl.useProgram(p);
  glErrorReport(arguments);
  return r;
}

function glGetAttribLocation(program, name, name_len)
{
  let str = wasm_read_string(lumenarium_wasm_instance, name, name_len);
  const r = gl.getAttribLocation(gl_get_program(program), str);
  glErrorReport(arguments);
  return r;
}

function glVertexAttribPointer(attr, size, type, normalized, stride, pointer)
{
  const r = gl.vertexAttribPointer(attr, size, type, normalized, stride, pointer);
  glErrorReport(arguments);
  return r;
}

function glEnableVertexAttribArray(index)
{
  const r = gl.enableVertexAttribArray(index);
  glErrorReport(arguments);
  return r;
}

function glDrawElements(type, index_count, ele_type, indices)
{
  const r = gl.drawElements(type, index_count, ele_type, indices);
  glErrorReport(arguments);
  return r;
}

function glGenTextures(count, ids_ptr, ids_size)
{
  for (let i = 0; i < count; i++)
  {
    const tex = gl.createTexture();
    glErrorReport(arguments);
    let new_len = glTextures.push(tex);
    put_u32(ids_ptr + (i * 4), new_len);
  }
}

function glBindTexture(slot, id)
{
  let tex = gl_get_texture(id);
  const r = gl.bindTexture(slot, tex);
  glErrorReport(arguments);
  return r;
}

function glTexParameteri(slot, param, value)
{
  const r = gl.texParameteri(slot, param, value);
  glErrorReport(arguments);
  return r;
}

function glTexImage2D(target, level, internalformat, width, height, border, format, type, data_ptr, data_size)
{
  const data = wasm_mem_get_u8_arr(lumenarium_wasm_instance, data_ptr, data_size);
  const r = gl.texImage2D(target, level, internalformat, width, height, border, format, type, data);
  glErrorReport(arguments);
  return r;
}

function glTexSubImage2D(target, level, offsetx, offsety, width, height, format, type, data_ptr, data_size)
{
  const data = wasm_mem_get_u8_arr(lumenarium_wasm_instance, data_ptr, data_size);
  const r = gl.texSubImage2D(target, level, offsetx, offsety, width, height, format, type, data);
  glErrorReport(arguments);
  return r;
}

function glGetUniformLocation(program, name, name_len)
{
  // TODO(PS): complete
  return 0;
}

function glUniformMatrix4fv()
{
  // TODO(PS): 
}

function webgl_add_imports (canvas_selector, imports) {
  const canvas = document.querySelector(canvas_selector);
  if (!canvas) return console.error("no canvas");
  
  gl = canvas.getContext("webgl2");
  if (gl === null) return console.error("no webgl ctx");
  
  imports.glHadError = () => { return gl_error; };
  imports.glClearColor = glClearColor;
  imports.glEnable = glEnable;
  imports.glDisable = glDisable;
  imports.glBlendFunc = glBlendFunc;
  imports.glViewport = glViewport;
  imports.glDepthFunc = glDepthFunc;
  imports.glClear = glClear;
  
  imports.glCreateBuffer = glCreateBuffer;
  imports.glBindBuffer = glBindBuffer;
  imports.glBufferData = glBufferData;
  imports.glBufferSubData = glBufferSubData;
  imports.glCreateShader = glCreateShader;
  imports.glShaderSource = glShaderSource;
  imports.glCompileShader = glCompileShader;
  imports.glCreateProgram = glCreateProgram;
  imports.glAttachShader = glAttachShader;
  imports.glLinkProgram = glLinkProgram;
  imports.glUseProgram = glUseProgram;
  imports.glGetAttribLocation = glGetAttribLocation;
  imports.glVertexAttribPointer = glVertexAttribPointer;
  imports.glEnableVertexAttribArray = glEnableVertexAttribArray;
  imports.glDrawElements = glDrawElements;
  imports.glGenTextures = glGenTextures;
  imports.glBindTexture = glBindTexture;
  imports.glTexParameteri = glTexParameteri;
  imports.glTexImage2D = glTexImage2D;
  imports.glTexSubImage2D = glTexSubImage2D;
  imports.glBindTexture = glBindTexture;
  imports.glGetUniformLocation = glGetUniformLocation;
  imports.glUniformMatrix4fv = glUniformMatrix4fv;
  
  return imports;
}