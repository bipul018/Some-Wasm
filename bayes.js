
const canv = document.getElementById('canv');
//canv.width = window.innerWidth;
//canv.height = window.innerHeight;
canv.width = 700;
canv.height = 850;
const document_dragger = create_drag_parent(document.body);
const canv_dragger = create_drag_parent(canv);

//Each element will be id: (int) and obj: (an html dom div object)
let divs = [];

function find_div_by_id(id){
    for(var i = 0; i < divs.length; i++){
	if(divs[i].id == id){
	    return divs[i];
	}
    }
    return null;
}

function draw_canvas(){
    const ctx = canv.getContext("2d");
    ctx.clearRect(0, 0, canv.width, canv.height);
    ctx.rect(0, 0, canv.width, canv.height);
    ctx.fillStyle = "blue";
    ctx.fill();
    
    ctx.rect(0, 0, canv.width, canv.height);
    ctx.fillStyle = "blue";
    ctx.fill();
    const cx = get_bounding_rect(canv).left;
    const cy = get_bounding_rect(canv).top;

    for(var i = 0; i < divs.length; i++){
	const div_id = divs[i].id;
	const node = decode_bayes_nodes(div_id);
	if(node == null)
	    continue;

	const child_rect = get_bounding_rect(divs[i].obj);
	
	const parents = new Int32Array(
	    wasm.memory.buffer,
	    node.parents,
	    node.parent_count);

	for(var j = 0; j < node.parent_count; j++){
	    const parent = find_div_by_id(parents[j]);
	    if(parent == null)
		continue;
	    const parent_rect = get_bounding_rect(parent.obj);
	    
	    const line = get_closest_line(parent_rect, child_rect);
	    ctx.strokeStyle = 'white';
	    ctx.fillStyle = 'white';
	    ctx.lineWidth = 5;
	    draw_arrow(ctx,
		       line.a.x-cx, line.a.y-cy,
		       line.b.x-cx, line.b.y-cy,
		       10);
	}
    }
    
}
draw_canvas();
window.addEventListener('scroll', draw_canvas);

let id1 = -1;
let id2 = -1;

function update_nodes_text(){
    const obj1 = decode_bayes_nodes(id1);
    const obj2 = decode_bayes_nodes(id2);
    if(obj1 != null)
	var name1 = obj1.node_name;
    else
	var name1 = ''
    if(obj2 != null)
	var name2 = obj2.node_name;
    else
	var name2 = ''

    document.getElementById('curr_nodes').textContent =
	'First selection id is  ' + id1 +" named '" +name1 +
	"' and second id is " + id2 + " named '" + name2 + "'";
}
document.getElementById('reset_selection').addEventListener('click',function(e){
    id1 = id2 = -1;
    update_nodes_text();
});
document.getElementById('join_node').addEventListener('click',function(e){
    if((id1 >= 0) && (id2 >= 0)){
	const div1 = find_div_by_id(id1);
	const div2 = find_div_by_id(id2);

	const added = wasm.insert_edge(div1.id, div2.id);
	if(added == false){
	    console.log("Couldn't add edge");
	}
	else{
	    console.log("Added an edge between object " + id1 + ' and object ' + id2);
	}
    }
    draw_canvas();
    id1 = id2 = -1;
    update_nodes_text();
});


document.getElementById('add_node').addEventListener('click',function(e){
    //First off read the name
    const new_name = document.getElementById('new_node_name').value;
    const cstr_name = make_c_str(new_name);
    const c_obj_id = wasm.create_new_node(cstr_name);
    if(c_obj_id < 0)
	return;
				     
    const new_obj = document.createElement('div');
    new_obj.innerText = new_name;
    new_obj.style.position = 'fixed';
    //new_obj.style.width = '50px';
    new_obj.style.height = '30px';
    new_obj.style.left = '100px';
    new_obj.style.top = '100px';
    new_obj.style.textAlign = 'center';
    new_obj.style.justifyContent = 'center';
    new_obj.style.padding = '3px';
    new_obj.style.backgroundColor = 'red';
    canv_dragger.make_draggable(new_obj);
    document.getElementById('all_nodes').appendChild(new_obj);
    new_obj.addEventListener('dblclick', function(e){
	if(id1 < 0){
	    id1 = c_obj_id;
	    create_prob_table(c_obj_id);
	}
	else if((id1 != c_obj_id) && (id2 < 0)){
	    id2 = c_obj_id;
	}
	update_nodes_text();
    });
    new_obj.addEventListener('mouse_drag', function(e){
		draw_canvas();
    });

    const div_obj = {
	id: c_obj_id,
	obj: new_obj
    };
    divs.push(div_obj);
    document.getElementById('new_node_name').value = '';
});

const table_div = document.getElementById('table_place');
document_dragger.make_draggable(table_div);
table_div.style.top = (get_bounding_rect(canv).top) + 'px';
table_div.style.left = (get_bounding_rect(canv).left + get_bounding_rect(canv).width - 200) + 'px'; 
var table_elem = null;
var table_id = -1;

function insert_prob(id, inx, new_val){
    const node = decode_bayes_nodes(id);
    if(node == null){
	console.log("Not a valid object id");
	return ;
    }
    const prob_size = (1 << node.parent_count);
    const prob_view = new Float32Array(
	wasm.memory.buffer,
	node.prob_dists,
	prob_size);
    if(inx < prob_size){
	prob_view[inx] = new_val;
    }
}

function create_prob_table(id){
    const node = decode_bayes_nodes(id);
    if(node == null){
	console.log("Not a valid object id");
	return ;
    }
    table_inx = id;
    const prob_size = (1 << node.parent_count);

    const parents = [];
    const parents_view = new Int32Array(
	wasm.memory.buffer,
	node.parents,
	node.parent_count);
    for(var j = 0; j < node.parent_count; j++){
	const parent = decode_bayes_nodes(parents_view[j]);
	parents.push(parent.node_name);
    }
    const tble_head_row = [[
	{textContent : "Combinations", isth : true},
	{textContent : "True Probability", isth : true}
    ]];
    const tble_head_col = [

    ];
    const tble_vals = [

    ];
    const prob_view = new Float32Array(
	wasm.memory.buffer,
	node.prob_dists,
	prob_size);
    for(var i = 0; i < prob_size; i++){
	var res = '';
	for(var j = 0; j < node.parent_count; j++){
	    const istrue = (0 != (i & (1 << j)));
	    res += "'" + parents[j] + ':' + istrue + "' ";
	}
	tble_head_col.push([{
	    textContent: res, isth : true
	}]);
	const thing = i;
	var entry = document.createElement('input');
	entry.type = 'number';
	entry.max = 1.0;
	entry.min = 0.0;
	entry.step = 0.01;
	entry.value = prob_view[i];
	function make_listner(eid, einx, eentry){
	    return function(e){
		insert_prob(eid, einx, Number(eentry.value));
	    }
	}
	entry.addEventListener('input', make_listner(id, i, entry));


	tble_vals.push([{child_node : entry}]);
    }
    table_div.replaceChildren();
    table_elem = make_table(tble_head_row, tble_head_col, tble_vals);
    
    table_div.appendChild(table_elem);
}

