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
  
  wasm_assert_always: () => { 
    console.assert(false); 
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

let gl = null;

function glClearColor (r, g, b, a) { gl.clearColor(r,g,b,a); }
function glEnable(v) { gl.enable(v); }
function glDisable(v) { gl.disable(v); }
function glBlendFunc(a,b) { gl.blendFunc(a,b); }
function glViewport(xmin, ymin, xmax, ymax) { gl.viewport(xmin,ymin,xmax,ymax); }
function glDepthFunc(v) { gl.depthFunc(v); }
function glClear(mask) { gl.clear(mask); }

function webgl_add_imports (canvas_selector, imports) {
  const canvas = document.querySelector(canvas_selector);
  if (!canvas) return console.error("no canvas");
  
  gl = canvas.getContext("webgl");
  if (gl === null) return console.error("no webgl ctx");
  
  ///////////////////////////////////////
  // Constants
  
  imports.GL_TEXTURE_2D = gl.TEXTURE_2D;
  imports.GL_BLEND = gl.BLEND;
  imports.GL_SRC_ALPHA = gl.SRC_ALPHA;
  imports.GL_ONE_MINUS_SRC_ALPHA = gl.ONE_MINUS_SRC_ALPHA;
  imports.GL_DEPTH_TEST = gl.DEPTH_TEST;
  imports.GL_LESS = gl.LESS;
  imports.GL_COLOR_BUFFER_BIT = gl.COLOR_BUFFER_BIT;
  imports.GL_DEPTH_BUFFER_BIT = gl.DEPTH_BUFFER_BIT;
  
  ///////////////////////////////////////
  // Functions
  
  imports.glClearColor = glClearColor;
  imports.glEnable = glEnable;
  imports.glDisable = glDisable;
  imports.glBlendFunc = glBlendFunc;
  imports.glViewport = glViewport;
  imports.glDepthFunc = glDepthFunc;
  imports.glClear = glClear;
  
  return imports;
}