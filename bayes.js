const canv = document.getElementById('canv');
//canv.width = window.innerWidth;
//canv.height = window.innerHeight;
canv.width = 1350;
canv.height = 750;
const document_dragger = create_drag_parent(document.body);
const canv_dragger = create_drag_parent(canv);

//Each element will be id: (int) and obj: (an html dom div object) and
//             and additional entry for joint probability
//             evaluation : desire_true, is_required, is_evidence
//             all boolean
//             and it's table_obj
//             and a boolean for whether to show table , 'show_table'

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
var probability_text = '';
let id1 = -1;
let id2 = -1;
const table_div = document.getElementById('table_place');
function update_nodes_text(){
    //Updates the tables' visibility
    //Updates the outputs
    table_div.replaceChildren();
    for(var i = 0; i < divs.length; ++i){
	if(divs[i].show_table){
	    table_div.appendChild(divs[i].table_obj);
	}
    }
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
	"' and second id is " + id2 + " named '" + name2 + "'\n" +
	probability_text;
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
	    const table_obj =  create_prob_table(div2.id);
	    div2.table_obj.replaceChildren();
	    div2.table_obj.appendChild(table_obj);
	}
    }
    draw_canvas();
    id1 = id2 = -1;
    update_nodes_text();
});

function calculate_probability(){
    const allocd = wasm.alloc_mem(divs.length * wasm.get_joint_entry_size());
    if(allocd == 0){
	console.log("Couldn't allocate memory for joint probability operation");
	return null;
    }
    const stride = wasm.get_joint_entry_size() / 4;
    const input_view = new Int32Array(
	wasm.memory.buffer,
	allocd,
	divs.length * stride);

    for(var i = 0; i < divs.length; ++i){
	//desire_true
	input_view[stride*i] = divs[i].desire_true?1:0;
	//is_required
	input_view[stride*i+1] = divs[i].is_required?1:0;
	//is_evidence
	input_view[stride*i+2] = divs[i].is_evidence?1:0;
	console.log('For ' + i +' : ' +
		    input_view[stride*i], input_view[stride*i+1],
		    input_view[stride*i+2]);
    }
    const result = wasm.process_conditional(allocd);
    console.log("The value obtained was : " + result);
    wasm.free_mem(allocd);
    return result;
}

document.getElementById("find_joint_prob").
    addEventListener('click',function(e){
	//Update probabilities
	probability_text = 'P(';
	var is_this_first = true;
	for(var i = 0; i < divs.length; ++i){
	    if(divs[i].is_required && !divs[i].is_evidence){
		const obj = decode_bayes_nodes(divs[i].id);
		if(!is_this_first)
		    probability_text += ', ';
		if(!divs[i].desire_true)
		    probability_text += 'not ';
		probability_text += "'" + obj.node_name +
		    "'";
		is_this_first = false;
	    }
	}
	probability_text += ' | ';
	is_this_first = true;
	for(var i = 0; i < divs.length; ++i){
	    if(divs[i].is_required && divs[i].is_evidence){
		const obj = decode_bayes_nodes(divs[i].id);
		if(!is_this_first)
		    probability_text += ', ';
		if(!divs[i].desire_true)
		    probability_text += 'not ';
		probability_text += "'" + obj.node_name +
		    "'";
		is_this_first = false;
	    }
	}
	probability_text += ') = ';
	const prob = calculate_probability();
	probability_text += prob;
	update_nodes_text();
    });


function bind_div_to_node(c_obj_id){

    if(c_obj_id < 0)
	return;
    const node_obj = decode_bayes_nodes(c_obj_id);
    const new_obj = document.createElement('div');
    new_obj.textContent = node_obj.node_name;
    const div_obj = {};
    
    //Add a radio for setting as hypothesis or evidence or neutral
    // P ( H1, H2,... | E1, E2, ...)
    new_obj.appendChild(document.createElement('br'));
    const hypothesis_button = document.createElement('input');
    hypothesis_button.type = 'radio';
    hypothesis_button.id = 'node_radio_hypothesis_for'+c_obj_id;
    hypothesis_button.name = 'node_radio_group1_for' + c_obj_id;
    hypothesis_button.addEventListener('change', (e)=>{
	if(hypothesis_button.checked){
	    div_obj.is_evidence = false;
	    div_obj.is_required = true;
	}
    });
    
    const hypothesis_label = document.createElement('label');
    hypothesis_label['for'] = hypothesis_button.id;
    hypothesis_label.textContent = 'As Hypothesis';

    const evidence_button = document.createElement('input');
    evidence_button.type = 'radio';
    evidence_button.id = 'node_radio_evidence_for' + c_obj_id;
    evidence_button.name = hypothesis_button.name;
    evidence_button.addEventListener('change', (e)=>{
	if(evidence_button.checked){
	    div_obj.is_evidence = true;
	    div_obj.is_required = true;
	}
    });
    
    const evidence_label = document.createElement('label');
    evidence_label['for'] = evidence_button.id;
    evidence_label.textContent = 'As Evidence';

    const neutral_button = document.createElement('input');
    neutral_button.type = 'radio';
    neutral_button.id = 'node_radio_neutral_for' + c_obj_id;
    neutral_button.name = evidence_button.name;
    neutral_button.checked = true;
    neutral_button.addEventListener('change', (e)=>{
	if(neutral_button.checked){
	    div_obj.is_required = false;
	}
	else{
	    div_obj.is_required = true;
	}
    });
    
    const neutral_label = document.createElement('label');
    neutral_label['for'] = neutral_button.id;
    neutral_label.textContent = "As Don't Care";

    const radio1_div = document.createElement('div');
    radio1_div.appendChild(hypothesis_button);
    radio1_div.appendChild(hypothesis_label);
    radio1_div.appendChild(evidence_button);
    radio1_div.appendChild(evidence_label);
    radio1_div.appendChild(neutral_button);
    radio1_div.appendChild(neutral_label);
    new_obj.appendChild(radio1_div);
    //Add event listeners
    
    //Add radiobuttons for true or false desire
    new_obj.appendChild(document.createElement('br'));
    
    const false_button = document.createElement('input');
    false_button.type = 'radio';
    false_button.id = 'node_radio_false_for' + c_obj_id;
    false_button.name = 'node_radio_group2_for' + c_obj_id;
    false_button.checked = true;
    false_button.addEventListener('change', (e)=>{
	if(false_button.checked){
	    div_obj.desire_true = false;
	}
    });
    const false_label = document.createElement('label');
    false_label['for'] = false_button.id;
    false_label.textContent = 'Choose False';

    const true_button = document.createElement('input');
    true_button.type = 'radio';
    true_button.id = 'node_radio_true_for' + c_obj_id;
    true_button.name = false_button.name;
    true_button.addEventListener('change', (e)=>{
	if(true_button.checked){
	    div_obj.desire_true = true;
	}
    });
    const true_label = document.createElement('label');
    true_label['for'] = true_button.id;
    true_label.textContent = 'Choose True';
    const radio2_div = document.createElement('div');
    radio2_div.appendChild(false_button);
    radio2_div.appendChild(false_label);
    radio2_div.appendChild(true_button);
    radio2_div.appendChild(true_label);
    new_obj.appendChild(radio2_div);

    //Need to create a checkmark
    new_obj.appendChild(document.createElement('br'));
    
    const table_check = document.createElement('input');
    table_check.type = 'checkbox';
    table_check.id = 'node_table_check_for' + c_obj_id;
    div_obj.show_table = false;
    table_check.addEventListener('change', ()=>{
	div_obj.show_table = !div_obj.show_table;
	update_nodes_text();
    });
    const table_label = document.createElement('label');
    table_label['for'] = table_check.id;
    table_label.textContent = 'Show Table';
    new_obj.appendChild(table_check);
    new_obj.appendChild(table_label);
    
    //Create Table :
    

    const this_table_div = document.createElement('div');
    document_dragger.make_draggable(this_table_div);
    this_table_div.style.top = (get_bounding_rect(canv).top) + 'px';
    this_table_div.style.left = (get_bounding_rect(canv).left + get_bounding_rect(canv).width - 200) + 'px';

    const table_obj =  create_prob_table(c_obj_id);
    this_table_div.appendChild(table_obj);
    div_obj.table_obj = this_table_div;
    
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

    table_elem = make_table(tble_head_row, tble_head_col, tble_vals);	
    return table_elem;
}

//Design a sample loading system
//simulae click on add_node
//Set the name for the node

//Each loading object will have almost everything that other side needs
//id, name
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
    
    for(var i = 0; i < data_things.length; ++i){
	const table_obj =  create_prob_table(divs[i].id);
	divs[i].table_obj.replaceChildren();
	divs[i].table_obj.appendChild(table_obj);
    }

}

const sample_data = [
    {node_id: 0, name: "Earthquake Happens",
     parents: [], prob_dists : [0.0]},
    {node_id: 1, name: "Burglary", 
     parents: [], prob_dists: [0.0]},
    {node_id: 2, name: "Alarm Rings",
     parents: [0,1], prob_dists: [0.0,0.2,0.5,0.8]},
    {node_id: 3, name: "John Calls",
     parents: [2], prob_dists: [0.5,0.5]},
    {node_id: 4, name: "Mary Calls",
     parents: [2], prob_dists: [0.2,0.8]}
];
const sprinkler_data = [
    {node_id: 0, name: "Rain", 
     parents: [], prob_dists: [0.2]},
    {node_id: 1, name: "Sprinkler", 
     parents: [0], prob_dists: [0.4,0.01]},
    {node_id: 2, name: "Grass Wet", 
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



const health_data = [{"node_id":0,"name":"Visit Asia?"
		      ,"parents":[]
		      ,"prob_dists":[0.009999999776482582]},
		     {"node_id":1,"name":"Has Tuberculosis"
		      ,"parents":[0]
		      ,"prob_dists":[0.009999999776482582,0.05000000074505806]},
		     {"node_id":2,"name":"Tuberculosis or Cancer"
		      ,"parents":[1,4]
		      ,"prob_dists":[0,1,1,1]},
		     {"node_id":3,"name":"Smoker"
		      ,"parents":[]
		      ,"prob_dists":[0.5]},
		     {"node_id":4,"name":"Has Lung Cancer"
		      ,"parents":[3]
		      ,"prob_dists":[0.009999999776482582,0.10000000149011612]},
		     {"node_id":5,"name":"Has Bronchitis"
		      ,"parents":[3]
		      ,"prob_dists":[0.30000001192092896,0.6000000238418579]},
		     {"node_id":6,"name":"Shortness of Breath"
		      ,"parents":[5,2]
		      ,"prob_dists":[0.10000000149011612,0.800000011920929,0.699999988079071,0.8999999761581421]},
		     {"node_id":7,"name":"Positive X-Ray"
		      ,"parents":[2]
		      ,"prob_dists":[0.05000000074505806,0.9800000190734863]}];


    
     
wasm_promise.then(()=>{
    //load_data(sample_data);
    //load_data(sprinkler_data);
    //load_data(family_eg);
    load_data(health_data);
});

//TODO:: make each one of them spawn a table
