
let module = null;
let instance = null;


async function load_webassembly_module ()
{
  lumenarium_wasm_imports = webgl_add_imports("#gl_canvas", lumenarium_wasm_imports);
  
  const path = "lumenarium.wasm";
  const promise = fetch(path);
  const module = await WebAssembly.compileStreaming(promise);
  
  let memory = new WebAssembly.Memory({ initial: 2 });
  const env = { 
    memory,
    ...lumenarium_wasm_imports,
  };
  
  let table = new WebAssembly.Table({ element: "anyfunc", initial: 32, });
  
  instance = await WebAssembly.instantiate(module, { env })
    .then((res, err) => {
            return res;
          })
    .catch((a, b) => {
             console.log(a,b);
           });
  lumenarium_wasm_instance = instance;
  
  // If function '__wasm_call_ctors' (global C++ constructors) exists, call it
  if (instance.exports.__wasm_call_ctors) instance.exports.__wasm_call_ctors();
  
  // If function 'main' exists, call it with dummy arguments
  let result = 0;
  if (instance.exports.main) result = instance.exports.main();
}

window.addEventListener("load", load_webassembly_module)
