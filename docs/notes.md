
# Notes

## Todo

- clear memory allocating funcs in mem allocator
- fix static vars, give them a ast scope instead of default value

- type checking on assign
- specify list of non-allowed variable/func/class names
- when using throw check function can_error
- concurrency
- concat strings
- improve refcounting


## Compile cache

recompile if:
- file changed
-> recompile dependency_for array

when to add to depency_for:
- when using class from other file
- when using a function from other file
- when using enum from other file

## Async
- No object sharing, only clones allowed

chan x = async this.respond(rslot);
=>
Async* tmp = malloc(Async);
tmp->func = &func;
tmp->args = malloc(ARGS_SIZE);
for(args as arg){
	if(arg is class|actor){
		arg->_RC++;
	}
	memcopy(tmp->args, arg, sizeof(arg));
	tmp->args += sizeof(arg);
}

@ result = await x;

## RC

Strategy:
- init object with rc = 0
- Every time a value that's a class instance with refcounting:
-> rc-- it's current value before assign + check if rc = 0, if so delete
-> rc++ after the assign
- How to return:
-> rc++ the return value and store it in _KI_RET
-> rc-- all local variables except _KI_RET
-> return _KI_RET;

// Example
A* a = something(); rc = 1
B* b = createB();
b->prop = a; rc = 2
b->prop = NULL; rc = 0; -> we cannot dealloc here because a still holds a reference

// deref a, so rc = 1;
// deref b, so rc = 0;
return;

## Syntax
@ list = array<i32>{};
@ list = []i32,10;

list.push(1);
@ z = list.get_index(55) or value 0;
@ m = Map<string,i32>{};
@ mv = m.get("yolo") or pass;
