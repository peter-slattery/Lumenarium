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
    let pages = new_size / WASM_PAGE_SIZE;
    let pages_rem = fract(pages);
    if (pages_rem > 0) pages = Math.floor(pages) + 1;
    let size_before = lumenarium_wasm_instance.exports.memory.buffer.byteLength;
    let old_page_count = lumenarium_wasm_instance.exports.memory.grow(pages);
    
    console.log("mem_grow\n", 
                "req size: ", new_size, "\n",
                "old size: ", (old_page_count * WASM_PAGE_SIZE), "\n",
                "old size: ", size_before, "\n",
                "grew by:  ", (pages * WASM_PAGE_SIZE), "\n", 
                "new size: ", lumenarium_wasm_instance.exports.memory.buffer.byteLength, "");
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
};

///////////////////////////////////////
// Web GL Imports

let gl = null;

// NOTE(PS): it seems like its not enough to set 
// the values of imports to gl.function
//   ie. imports.glClearColor = gl.clearColor
// instead we need to wrap them for some reason. 
// Not sure why
function glClearColor (r, g, b, a) { return gl.clearColor(r,g,b,a); }
function glEnable(v) { return gl.enable(v); }
function glDisable(v) { return gl.disable(v); }
function glBlendFunc(a,b) { return gl.blendFunc(a,b); }
function glViewport(xmin, ymin, xmax, ymax) { return gl.viewport(xmin,ymin,xmax,ymax); }
function glDepthFunc(v) { return gl.depthFunc(v); }
function glClear(mask) { return gl.clear(mask); }

let glBuffers = [];
let glShaders = [];
let glPrograms = [];
function gl_get_managed_resource(arr, id) {
  if (id == 0) return null;
  return arr[id - 1];
}
function gl_get_buffer(id) { return gl_get_managed_resource(glBuffers, id); }
function gl_get_shader(id) { return gl_get_managed_resource(glShaders, id); }
function gl_get_program(id) { return gl_get_managed_resource(glPrograms, id); }

function glCreateBuffer() { 
  let buffer = gl.createBuffer(); 
  let new_len = glBuffers.push(buffer);
  return new_len;
}
function glBindBuffer(buffer_kind, buffer_id) 
{
  return gl.bindBuffer(buffer_kind, gl_get_buffer(buffer_id));
}
function glBufferData(target, size, ptr, usage)
{
  let data = wasm_mem_get_u8_arr(lumenarium_wasm_instance, ptr, size);
  return gl.bufferData(target, data, usage);
}
function glCreateShader(kind)
{
  let shader = gl.createShader(kind);
  let new_len = glShaders.push(shader);
  return new_len;
}
function glShaderSource(shader_id, shader_code, shader_code_len)
{
  let str = wasm_read_string(lumenarium_wasm_instance, shader_code, shader_code_len);
  console.error("For some reason, str isn't getting the correct data out of here", str);
  return gl.shaderSource(gl_get_shader(shader_id), str);
}
function glCompileShader(shader_id)
{
  let s = gl_get_shader(shader_id);
  let r = gl.compileShader(s);
  let m = gl.getShaderInfoLog(s);
  if (m.length > 0)
  {
    console.error("glCompileShader: \n\n" + m);
  }
}
function glCreateProgram()
{
  let prog = gl.createProgram();
  let new_len = glPrograms.push(prog);
  return new_len;
}
function glAttachShader(program, shader)
{
  let s = gl_get_shader(shader);
  let p = gl_get_program(program);
  return gl.attachShader(p, s);
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
  return gl.useProgram(p);
}
function glGetAttribLocation(program, name, name_len)
{
  let str = wasm_read_string(lumenarium_wasm_instance, name, name_len);
  return gl.getAttribLocation(gl_get_program(program), str);
}
function glVertexAttribPointer(attr, size, type, normalized, stride, pointer)
{
  return gl.vertexAttribPointer(attr, size, type, normalized, stride, pointer);
}
function glEnableVertexAttribArray(index)
{
  return gl.enableVertexAttribArray(index);
}
function glDrawElements(type, index_count, ele_type, indices)
{
  return gl.drawElements(type, index_count, ele_type, indices);
}

function webgl_add_imports (canvas_selector, imports) {
  const canvas = document.querySelector(canvas_selector);
  if (!canvas) return console.error("no canvas");
  
  gl = canvas.getContext("webgl");
  if (gl === null) return console.error("no webgl ctx");
  
  console.log(
              gl.FLOAT.toString(16), "\n",
              gl.UNSIGNED_INT.toString(16), "\n"
              );
  
  
  
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
  
  return imports;
}