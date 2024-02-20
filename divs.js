
//top bottom left right based rect
//But right >= left, bottom >= top
function get_closest_line(rect1, rect2){

    if(rect1.left > rect2.left){
	const obj = get_closest_line(rect2, rect1);
	return {a : obj.b, b : obj.a};
    }
    let line = {
	a : {
	    x : undefined,
	    y : undefined 
	},
	b : {
	    x : undefined,
	    y : undefined
	}
    };

    if(rect2.left > rect1.right){
	line.a.x = rect1.right; line.b.x = rect2.left;
    }
    else{
	line.a.x = line.b.x = (Math.max(rect1.left, rect2.left) +
			       Math.min(rect1.right, rect2.right)) / 2;
    }

    if(rect2.top > rect1.bottom){
	line.a.y = rect1.bottom; line.b.y = rect2.top;
    }
    else if(rect2.bottom < rect1.top){
	line.a.y = rect1.top; line.b.y = rect2.bottom;
    }
    else{
	line.a.y = line.b.y = (Math.min(rect1.bottom, rect2.bottom) +
			       Math.max(rect1.top, rect2.top)) / 2;
    }
    return line;
}

function translateit(elem, dx, dy){
    const offs = elem.getBoundingClientRect();
    elem.style.left = (offs.left + dx) + "px";
    elem.style.top = (offs.top + dy) + "px";
}

function fix_one_in_other(inner_elem, bound_elem){
    const bound = bound_elem.getBoundingClientRect();
    const inner = inner_elem.getBoundingClientRect();

    if(inner.left < bound.left){
	translateit(inner_elem, bound.left - inner.left, 0);
    }
    if(inner.top < bound.top){
	translateit(inner_elem, 0, bound.top - inner.top);
    }
    if(inner.right > bound.right){
	translateit(inner_elem, bound.right - inner.right, 0);
    }
    if(inner.bottom > bound.bottom){
	translateit(inner_elem, 0, bound.bottom - inner.bottom);
    }
}

function draw_arrow(ctx, fromx, fromy, tox, toy, head_len){
    const prevstroke = ctx.strokeStyle;
    var angle;
    var x;
    var y;
    angle = Math.atan2(toy-fromy,tox-fromx);
    x = tox; y = toy;
    x -= head_len * Math.cos(angle); y -= head_len * Math.sin(angle);
    
    ctx.strokeStyle = ctx.fillStyle;
    ctx.beginPath();
    ctx.moveTo(fromx, fromy);
    ctx.lineTo(x, y);
    ctx.stroke();
    
    
    const prevwid = ctx.lineWidth;
    ctx.lineWidth = 1;
    
    ctx.beginPath();
    
    ctx.moveTo(tox, toy);


    x += prevwid * 1.5 * Math.sin(angle);
    y += -prevwid* 1.5 * Math.cos(angle);
    ctx.lineTo(x, y);
    
    x -= prevwid * 3 * Math.sin(angle);
    y -= -prevwid* 3 * Math.cos(angle);
    ctx.lineTo(x, y);
    ctx.closePath();
    ctx.fill();
    ctx.strokeStyle = prevstroke;
    ctx.lineWidth = prevwid;
}

function create_drag_parent(elem){
    const obj =  {
	curr_obj : null,
	own : elem,
	move_func : null,
	make_draggable : null
    }
    obj.move_func = function(e){
	if(null != obj.curr_obj){
	    const prev = obj.curr_obj.getBoundingClientRect();
	    translateit(obj.curr_obj, e.movementX, e.movementY);
	    fix_one_in_other(obj.curr_obj, obj.own);
	    const next = obj.curr_obj.getBoundingClientRect();
	    const event = new CustomEvent('mouse_drag',
					  {bubbles: true,
					   detail: {dax : -1, day : 29}});
//					  {dx : next.left - prev.left,
//					   dy : next.right - prev.right});
	    obj.curr_obj.dispatchEvent(event);
	}
    };
    obj.make_draggable = function(item){
	item.addEventListener('mousedown', function(e){
	    obj.curr_obj = item;
	    document.removeEventListener('mousemove', obj.move_func);
	    document.addEventListener('mousemove', obj.move_func);
	})
	item.className = item.className + ' prevent-select';
    };
    document.addEventListener('mouseup', function(e){
	document.removeEventListener('mousemove', obj.move_func);
	obj.curr_obj = null;
    });
    return obj;
}

let mouse_pos={
    x:0,
    y:0
};


document.addEventListener('mousemove', function(e){
    mouse_pos.x = e.x;
    mouse_pos.y = e.y;
});



function process_table_cell(cell_thing){
    var cell = null;
    if((typeof(cell_thing)=="object") &&
       (cell_thing["isth"] == true)){
	cell = document.createElement('th');
    }
    else{
	cell = document.createElement('td');
    }
    var child_node = null;
    if(typeof(cell_thing) == 'object'){
	for (let [key, item] of Object.entries(cell_thing)){
	    if(key == 'child_node'){
		child_node = item;
	    }
	    else if(key != 'isth'){
		cell[key] = item;
	    }
	}
    }
    else{
	cell.textContent += cell_thing;
    }
    if (child_node != null){
	cell.appendChild(child_node);
    }
    return cell;
}

function make_table(sampleh, samplel, sampled){
    let tble = document.createElement('table');
    for(var i = 0; i < sampleh.length; i++){
	var row = document.createElement('tr');
	for(var j = 0; j < sampleh[i].length; j++){
	    row.appendChild(process_table_cell(sampleh[i][j]));
	}
	tble.appendChild(row);
    }

    for(var i = 0; i < Math.max(samplel.length, sampled.length); i++){
	var row = document.createElement('tr');
	if(i < samplel.length){
	    for(var j = 0; j < samplel[i].length; j++){
		row.appendChild(process_table_cell(samplel[i][j]));
	    }
	}
	if(i < sampled.length){
	    for(var j = 0; j < sampled[i].length; j++){
		row.appendChild(process_table_cell(sampled[i][j]));
	    }
	}
	tble.appendChild(row);
    }
    return tble;
}

function create_num_inputs(rows, cols, min, max, step){
    var res = [];
    for(var i = 0; i < rows; i++){
	var row = [];
	for(var j = 0; j < cols; j++){
	    var elem = document.createElement('input');
	    elem.type = 'number';
	    elem.max = max;
	    elem.min = min;
	    elem.step = step;
	    row[j] = {child_node: elem};
	}
	res[i] = row;
    }
    return res;
}

function extract_num_inputs(arr){
    var res = [];
    for(var i = 0; i < arr.length; i++){
	res[i] = [];
	for(var j = 0; j < arr[i].length; j++){
	    res[i][j] = Number(arr[i][j].child_node.value);
	}
    }
    return res;
}
