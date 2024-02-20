
let wasm_inst;
let wasm;
var allocated_c_strs =[];

//Just allocates a c string in the wasm
function alloc_c_str(str){
    if (typeof (str) === 'string') {
	const cstr = wasm.alloc_mem(str.length + 1);
	if(cstr == 0)
	    return 0;
	const view = new Uint8Array(
	    wasm.memory.buffer,
	    cstr,
	    str.length + 1
	);
	for(var i = 0; i < str.length; i++){
	    view[i] = str.charCodeAt(i);
	}
	view[str.length] = 0;
	return cstr;
    }
    else{
	return 0;
    }
}
//Allocates a c string in wasm and pushes it in an array if needed to clean later
function make_c_str(str){
    const allocd = alloc_c_str(str);
    if(allocd != 0){
	allocated_c_strs.push(allocd);
    }
    return allocd;
}
function free_all_c_str(){
    allocated_c_strs.forEach((item) =>
	wasm.free_mem(item));
    allocated_c_strs = [];
}

function from_c_str(c_str){
    const view = new Uint8Array(
	wasm_inst.exports.memory.buffer,
	c_str);

    res_str = '';
    var inx = 0;
    while(view[inx] != 0){
	res_str += String.fromCharCode(view[inx]);
	inx++;
    }
    return res_str;
}

async function init() {

    const {instance } = await WebAssembly.instantiateStreaming(
	fetch("bayes.wasm"),
	{
	    "env": {
		logint: function(intval) {console.log(intval);},
		grow_memory_by_page:  function(count){console.log(' Request to grow memory by ' + count + ' pages');instance.exports.memory.grow(count)},
		log_c_str: function(c_str){   
		    console.log(from_c_str(c_str));
		}
	    }
	}
    );
    wasm_inst = instance;
    wasm = wasm_inst.exports;
    wasm.init_wasm();
    
}
init().then(()=>{
    
});
function decode_bayes_node_ptr(node_ptr){
    if(node_ptr == 0)
	return null;
    const view = new Uint32Array(
	wasm.memory.buffer,
	node_ptr,
	//Hardcoded from the bayesnode struct,
	//The five elements are node_name, parent_count, prob_dist array, parents array, id
	5
    );
    var obj = {};
    obj.node_name = from_c_str(view[0]);
    obj.parent_count = view[1];
    obj.prob_dists = view[2];
    obj.parents = view[3];
    obj.node_id = view[4];
    return obj;
}
//Now bayes related functions
function decode_bayes_nodes(node_id){
    const node_ptr = wasm.find_node(node_id);
    return decode_bayes_node_ptr(node_ptr);
}
function decode_bayes_node_arr(node_base_ptr, count){
    var res = [];
    for(var i = 0; i < count; i++){
	res.push(decode_bayes_node_ptr(node_base_ptr + 4 * 5 * i));
    }
    return res;
}

