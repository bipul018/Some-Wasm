
//let instance= undefined;
let g_instance;
let wins;
let canv_mem_ptr;
let canv_mem_size;
var allocated_c_strs =[];
function make_c_str(str){
    if (typeof str === 'string') {
	const cstr = wins.alloc_mem(str.length + 1);
	if(cstr == 0)
	    return 0;
	const view = new Uint8Array(
	    wins.memory.buffer,
	    cstr,
	    str.length + 1
	);
	for(var i = 0; i < str.length; i++){
	    view[i] = str.charCodeAt(i);
	}
	view[str.length] = 0;
	allocated_c_strs.push(cstr);
	return cstr;
    }
    else{
	return 0;
    }
}
function free_all_c_str(){
    allocated_c_strs.forEach((item) =>
	wins.free_mem(item));
    allocated_c_strs = [];
}
function log_c_str(c_str){
    const view = new Uint8Array(
	g_instance.exports.memory.buffer,
	c_str);

    res_str = '';
    var inx = 0;
    while(view[inx] != 0){
	res_str += String.fromCharCode(view[inx]);
	inx++;
    }
    console.log(res_str);
}
const canvas = document.getElementById("canvas");
const ctx = canvas.getContext("2d");

async function init() {

    const {instance } = await WebAssembly.instantiateStreaming(
	fetch("t1.wasm"),
	{
	    "env": {
		logint: function(intval) {console.log(intval);},
		grow_memory_by_page:  function(count){console.log(' Request to grow memory by ' + count + ' pages');instance.exports.memory.grow(count)},
		log_c_str: log_c_str
	    }
	}
    );
    g_instance = instance;
    wins = g_instance.exports;
    wins.init_wasm();
    ctx.width = canvas.clientWidth;
    ctx.height = canvas.clientHeight;
    console.log('Canvas width : ' + ctx.width + ' Canvas height ' + ctx.height);
    canv_mem_size = ctx.width * ctx.height * 4;
    canv_mem_ptr = instance.exports.alloc_mem(canv_mem_size);
    console.log('Canvas memory size : ' + canv_mem_size);
    
    
}
init().then(()=>{
    setInterval(update, 80);
});

function recreate_canvas(new_wid, new_hei){
    //console.log('Old memory pointer inx ' + canv_mem_ptr);
    console.log('\n');
    const new_canv_mem_size = new_wid * new_hei * 4;
    const new_mem = wins.realloc_mem(canv_mem_ptr, new_canv_mem_size);
    //console.log('New memory pointer inx ' + new_mem);
    //wins.free_mem(canv_mem_ptr);
    //new_mem = wins.alloc_mem(canv_mem_size);
    //canv_mem_ptr = wins.alloc_mem(canv_mem_size);

    if(new_mem != 0){
	canvas.width = new_wid;
	canvas.height = new_hei;
	ctx.width = new_wid;
	ctx.height = new_hei;
	canv_mem_ptr = new_mem;
	canv_mem_size = new_canv_mem_size;
    }
    console.log('Canvas width : ' + ctx.width + ' Canvas height ' + ctx.height);
}
function inc_height(){
    if(ctx.height <  900)
	recreate_canvas(ctx.width, ctx.height + 10);
}
function dec_height(){
    if(ctx.height >  100)
	recreate_canvas(ctx.width, ctx.height - 10);
}
function inc_width(){
    if(ctx.width <  1000)
	recreate_canvas(ctx.width + 10, ctx.height);
}
function dec_width(){
    if(ctx.width >  100)
	recreate_canvas(ctx.width - 10, ctx.height);
}
function update(){

    
    g_instance.exports.update_canvas(ctx.width, ctx.height, canv_mem_ptr);
    //g_instance.exports.update_canvas(ctx.width, ctx.height);
    const view = new Uint8ClampedArray(
	g_instance.exports.memory.buffer,
	canv_mem_ptr,
	canv_mem_size);
    
    img = new ImageData(view,ctx.width, ctx.height);
    ctx.putImageData(img, 0, 0);
    
}
