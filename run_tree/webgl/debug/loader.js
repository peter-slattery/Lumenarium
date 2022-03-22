
let memory = null;
let memory_u8 = null;

// TODO(PS): you need to figure out how to get text to convert to a string
function print (text) 
{
  console.log(i, memory_u8.length, text);
}

async function load_webassembly_module ()
{
  const path = "lumenarium.wasm";
  const promise = fetch(path);
  const module = await WebAssembly.compileStreaming(promise);
  memory = new WebAssembly.Memory({ initial: 2 });
  memory_u8 = new Uint8Array(memory.buffer);
  const env = { 
    memory,
    print,
  };
  const result  = await WebAssembly.instantiate(module, { env });
  console.log(result);
  
  result.exports.main();
  
}

window.addEventListener("load", load_webassembly_module)


/*
function load_webassembly_module (wa)
{
  fetch(wa.module)
    .then(res => res.arrayBuffer())
    .then((wasm_bytes) => {
            "use strict";
            wasm_bytes = new Uint8Array(wasm_bytes);
            
            // find start of heap and calc initial mem requirements
            var wasm_heap_base = 65536;
            
            // see https://webassembly.org/docs/binary-encoding/
            for (let i = 8, section_end, type, length; 
                 i < wasm_bytes.length; 
                 i = section_end
                 ){
              
              function get_b8() 
              { 
                return wasm_bytes[i++]; 
              }
              
              function get_leb() 
              { 
                for (var s = i, r = 0, n = 128; n & 128; i++)
                {
                  r |= ((n = wasm_bytes[i]) & 127) << ((i - s) * 7);
                }
                return r; // TODO(PS): this might belong in the for loop?
              }
              
              type   = get_leb();
              length = get_leb();
              section_end = i + length;
              
              if (type < 0 || type > 11 || length <= 0 || section_end > wasm_bytes.length)
              {
                break;
              }
              
              if (type == 6)
              {
                //Section 6 'Globals', llvm places the heap base pointer into the first value here
                let count = get_leb();
                let gtype = get_b8(); 
                let mutable = get_b8(); 
                let opcode = get_leb(); 
                let offset = get_leb(); 
                let endcode = get_leb();
                wasm_heap_base = offset;
                break;
              }
            }
            
            // Set the wasm memory size to [DATA] + [STACK] + [256KB HEAP]
            // (This loader does not support memory growing so it stays at this size)
            var wasm_mem_init = ((wasm_heap_base + 65535) >> 16 << 16) + (256 * 1024);
            var env = {
              env: { 
                memory: new WebAssembly.Memory(
                                               {
                                                 initial: wasm_mem_init >> 16 
                                               }
                                               ),
              },
            };
            
            // Instantiate the wasm module by passing the prepared environment
            WebAssembly.instantiate(wasm_bytes, env)
              .then(function (output)
                    {
                      // Store the list of the functions exported by the wasm module in WA.asm
                      wa.asm = output.instance.exports;
                      
                      // If function '__wasm_call_ctors' (global C++ constructors) exists, call it
                      if (wa.asm.__wasm_call_ctors) wa.asm.__wasm_call_ctors();
                      
                      // If function 'main' exists, call it with dummy arguments
                      if (wa.asm.main) wa.asm.main(0, 0);
                      
                      // If the outer HTML file supplied a 'started' callback, call it
                      if (wa.started) wa.started();
                    })
              .catch(function (err)
                     {
                       // On an exception, if the err is 'abort' the error was already processed in the abort function above
                       if (err !== 'abort') {
                         let stack = err.stack ? "\n" + err.stack : "";
                         console.error('BOOT: WASM instiantate error: ' + err + stack);
                         return;
                       }
                     });
          });
}

load_webassembly_module(web_assembly)

*/