
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
		},
		logdouble: function(dval){console.log(dval);}
	    }
	}
    );
    wasm_inst = instance;
    wasm = wasm_inst.exports;
    wasm.init_wasm();
    
}
const wasm_promise = init();
wasm_promise.then(()=>{
});
function decode_bayes_node_ptr(node_ptr){
    if(node_ptr == 0)
	return null;
    const view = new Uint32Array(
	wasm.memory.buffer,
	node_ptr,
	//Hardcoded from the bayesnode struct,
	//The five elements are node_name, parent_count, prob_dist array, parents array, id
	Math.floor(wasm.get_bayes_node_size() / 4)
    );
    var obj = {};
    obj.node_id = view[4];
    obj.node_name = from_c_str(view[0]);
    //Last element of view points to float value
    obj.evidence_prob = wasm.get_evidence_prob(obj.node_id);
    //Just after the last element of view is bool value
    obj.has_evidence = wasm.get_has_evidence(obj.node_id);
    obj.parent_count = view[1];
    obj.prob_dists = view[2];
    obj.parents = view[3];

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
	res.push(decode_bayes_node_ptr(node_base_ptr + wasm.get_bayes_node_size() * i));
    }
    return res;
}

function extract_whole_node(node_ptr, node_count){

    const nodes = [];
    for(var i = 0; i < node_count; i++){
	nodes.push(decode_bayes_node_ptr(node_ptr + wasm.get_bayes_node_size() * i));
    }
    const res = [];
    for(var i = 0; i < node_count; i++){
	const probs = [];
	const sides = [];
	const prob_count = (1 << nodes[i].parent_count);
	const prob_view = new Float32Array(
	    wasm.memory.buffer,
	    nodes[i].prob_dists,
	    prob_count);
	const sides_view = new Uint32Array(
	    wasm.memory.buffer,
	    nodes[i].parents,
	    nodes[i].parent_count);
	for(var j = 0; j < nodes[i].parent_count; j++){
	    sides.push(sides_view[j]);
	}
	for(var j = 0; j < prob_count; j++){
	    probs.push(prob_view[j]);
	}
	res.push({
	    node_id: nodes[i].node_id,
	    name: nodes[i].node_name,
	    has_evidence: nodes[i].has_evidence,
	    evidence_prob: nodes[i].evidence_prob,
	    parents: sides,
	    prob_dists: probs
	});
    }
    
    return res;
}

