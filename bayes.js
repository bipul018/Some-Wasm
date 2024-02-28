
const canv = document.getElementById('canv');
//canv.width = window.innerWidth;
//canv.height = window.innerHeight;
canv.width = 950;
canv.height = 750;
const document_dragger = create_drag_parent(document.body);
const canv_dragger = create_drag_parent(canv);

//Each element will be id: (int) and obj: (an html dom div object) and
//             and additional entry for joint probability
//             evaluation : desire_true, and is_required
//             both boolean
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
    // ctx.rect(0, 0, canv.width, canv.height);
    // ctx.fillStyle = "blue";
    // ctx.fill();
    
    // ctx.rect(0, 0, canv.width, canv.height);
    // ctx.fillStyle = "blue";
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
	    ctx.strokeStyle = 'blue';
	    ctx.fillStyle = 'blue';
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
    table_div.replaceChildren();
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

document.getElementById("find_joint_prob").
    addEventListener('click',function(e){
	const allocd = wasm.alloc_mem(divs.length * 4 * 2);
	if(allocd == 0){
	    console.log("Couldn't allocate memory for joint probability operation");
	    return null;
	}

	const input_view = new Int32Array(
	    wasm.memory.buffer,
	    allocd,
	    divs.length * 2);

	for(var i = 0; i < divs.length; ++i){
	    //desire_true
	    input_view[2*i] = divs[i].desire_true?1:0;
	    //is_required
	    input_view[2*i+1] = divs[i].is_required?1:0;
	}
	console.log('The int val array going to be used : ' + input_view);
	const result = wasm.process_joint(allocd);
	console.log("The value obtained was : " + result);
	wasm.free_mem(allocd);
	return result;
    });


function bind_div_to_node(c_obj_id){

    if(c_obj_id < 0)
	return;
    const node_obj = decode_bayes_nodes(c_obj_id);
    const new_obj = document.createElement('div');
    new_obj.textContent = node_obj.node_name;
    const div_obj = {};
    //Add a check mark for fixing as evidence
    new_obj.appendChild(document.createElement('br'));
    const check = document.createElement('input');
    check.type = 'checkbox';
    check.checked = new_obj.has_evidence;
    check.id = 'node_checkbox_for' + c_obj_id;
    check.addEventListener('change', function(e){
	console.log("In the 'change' one " + c_obj_id);
	const val = decode_bayes_nodes(c_obj_id);
	if(val.has_evidence){
	    wasm.reset_has_evidence(c_obj_id);
	}
	else{
	    wasm.set_has_evidence(c_obj_id);
	}
	if(id1 == c_obj_id){
	    create_prob_table(c_obj_id);
	}
    });
    new_obj.appendChild(check);
    const check_label = document.createElement('label');
    check_label['for'] = check.id;
    check_label.textContent = 'Has Evidence';
    new_obj.appendChild(check_label);

    
    //Add radiobuttons
    new_obj.appendChild(document.createElement('br'));
    
    const false_button = document.createElement('input');
    false_button.type = 'radio';
    false_button.id = 'node_radio_false_for' + c_obj_id;
    false_button.value = 0;
    false_button.name = 'node_radio_group_for' + c_obj_id;
    const false_label = document.createElement('label');
    false_label['for'] = false_button.id;
    false_label.textContent = 'Choose False';

    const true_button = document.createElement('input');
    true_button.type = 'radio';
    true_button.id = 'node_radio_true_for' + c_obj_id;
    true_button.value = 1;
    true_button.name = false_button.name;
    const true_label = document.createElement('label');
    true_label['for'] = true_button.id;
    true_label.textContent = 'Choose True';

    
    const neutral_button = document.createElement('input');
    neutral_button.type = 'radio';
    neutral_button.id = 'node_radio_neutral_for' + c_obj_id;
    neutral_button.value = 2;
    neutral_button.name = false_button.name;
    neutral_button.checked = true;
    const neutral_label = document.createElement('label');
    neutral_label['for'] = neutral_button.id;
    neutral_label.textContent = "Don't Care";

    const change_func = (e)=>{
	if(false_button.checked)
	    div_obj.desire_true = false;
	if(true_button.checked)
	    div_obj.desire_true = true;
	if(neutral_button.checked)
	    div_obj.is_required = false;
	else
	    div_obj.is_required = true;
    };
    false_button.addEventListener('change', change_func);
    true_button.addEventListener('change', change_func);
    neutral_button.addEventListener('change', change_func);
    const radio_div = document.createElement('div');
    radio_div.appendChild(false_button);
    radio_div.appendChild(false_label);
    radio_div.appendChild(true_button);
    radio_div.appendChild(true_label);
    radio_div.appendChild(neutral_button);
    radio_div.appendChild(neutral_label);
    
    new_obj.appendChild(radio_div);
    
    new_obj.style.position = 'fixed';
    //new_obj.style.width = '50px';
    //new_obj.style.height = '30px';
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

    div_obj.id = c_obj_id;
    div_obj.obj = new_obj;
    
    divs.push(div_obj);
    document.getElementById('new_node_name').value = '';
}

document.getElementById('add_node').addEventListener('click',function(e){
    //First off read the name
    const new_name = document.getElementById('new_node_name').value;
    const cstr_name = make_c_str(new_name);
    const c_obj_id = wasm.create_new_node(cstr_name);
    bind_div_to_node(c_obj_id);
});

const table_div = document.getElementById('table_place');
document_dragger.make_draggable(table_div);
table_div.style.top = (get_bounding_rect(canv).top) + 'px';
table_div.style.left = (get_bounding_rect(canv).left + get_bounding_rect(canv).width - 200) + 'px'; 
var table_elem = null;
var table_id = -1;

function change_prob(id, inx, new_val){
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
    if(node.has_evidence){

	tble_head_col.push([{
	    textContent: 'Evidence for ' + node.node_name , ith : true}]);

	const entry = document.createElement('input');
	entry.type = 'number';
	entry.max = 1.0;
	entry.min = 0.0;
	entry.step = 0.01;
	entry.value = node.evidence_prob;
	entry.addEventListener('input', function (e){
	    wasm.set_evidence_prob(node.node_id, Number(entry.value));
	});
	tble_vals.push([{child_node : entry}]);
    }
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
		change_prob(eid, einx, Number(eentry.value));
	    }
	}
	entry.addEventListener('input', make_listner(id, i, entry));


	tble_vals.push([{child_node : entry}]);
    }
    table_div.replaceChildren();
    table_elem = make_table(tble_head_row, tble_head_col, tble_vals);
    
    table_div.appendChild(table_elem);
}

//Design a sample loading system
//simulae click on add_node
//Set the name for the node

//Each loading object will have almost everything that other side needs
//id, name, evidenceprob, has evidence bool , then
//it has a list of parents ids, then a list of probability distributions

//The data loading may change ids though

function load_data(data_things){
    const loaded_ids = [];
    //Insert nodes
    for(var i = 0; i < data_things.length; ++i){
	const cstr_name = make_c_str(data_things[i].name);
	loaded_ids[i] = wasm.create_new_node(cstr_name);
	if(loaded_ids[i] < 0)
	    continue;
	bind_div_to_node(loaded_ids[i]);
	wasm.set_evidence_prob(loaded_ids[i], data_things[i].evidence_prob);
	if(data_things.has_evidence)
	    wasm.set_has_evidence(loaded_ids[i]);
	else
	    wasm.reset_has_evidence(loaded_ids[i]);
	
    }
    function find_by_old_id(old_id){
	for(var i = 0; i < data_things.length; ++i){
	    if(data_things[i].node_id  === old_id)
		return loaded_ids[i];
	}
	return -1;
    }
    //Insert parents
    for(var i = 0; i < data_things.length; ++i){
	for(var j = 0; j < data_things[i].parents.length; ++j){
	    const added = wasm.insert_edge(find_by_old_id(data_things[i].parents[j]),
					   loaded_ids[i]);
	}
    }

    //Change the prob dist table
    //This will be wrong if for any reason any allocation in above sections break
    for(var i = 0; i < data_things.length; ++i){
	for(var j = 0; j < data_things[i].prob_dists.length; ++j){
	    change_prob(loaded_ids[i], j, data_things[i].prob_dists[j]);
	}
    }
}

const sample_data = [
    {node_id: 0, name: "Earthquake Happens", has_evidence: false, evidence_prob: 0.0,
     parents: [], prob_dists : [0.0]},
    {node_id: 1, name: "Burglary", has_evidence: false, evidence_prob: 0.0,
     parents: [], prob_dists: [0.0]},
    {node_id: 2, name: "Alarm Rings", has_evidence: false, evidence_prob: 0.0,
     parents: [0,1], prob_dists: [0.0,0.2,0.5,0.8]},
    {node_id: 3, name: "John Calls", has_evidence: false, evidence_prob: 0.0,
     parents: [2], prob_dists: [0.5,0.5]},
    {node_id: 4, name: "Mary Calls", has_evidence: false, evidence_prob: 0.0,
     parents: [2], prob_dists: [0.2,0.8]}
];
const sprinkler_data = [
    {node_id: 0, name: "Rain", has_evidence: false, evidence_prob: 0.0,
     parents: [], prob_dists: [0.2]},
    {node_id: 1, name: "Sprinkler", has_evidence: false, evidence_prob: 0.0,
     parents: [0], prob_dists: [0.4,0.01]},
    {node_id: 2, name: "Grass Wet", has_evidence: false, evidence_prob: 0.0,
     parents: [0,1], prob_dists: [0.0,0.8,0.9,0.99]}
];
const family_eg = [
    {node_id: 0, name: "Family Out", prob_dists:[0.15], parents: []},
    {node_id: 1, name: "Light On", parents: [0], prob_dists: [0.05, 0.6]},
    {node_id: 2, name: "Bowel Problem", parents: [], prob_dists: [0.01]},
    {node_id: 3, name: "Dog Out", parents:[0, 2],
     prob_dists: [0.3, 0.90, 0.97, 0.99]},
    {node_id: 4, name: "Hear Bark", parents:[3], prob_dists: [0.01, 0.7]}
];
     
    
    
     
wasm_promise.then(()=>{
    //load_data(sample_data);
    //load_data(sprinkler_data);
    load_data(family_eg);
});
