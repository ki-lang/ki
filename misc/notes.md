
# Notes

## Stores

local var
global
object property
array-of
union

## Returning value | upref/deref

When returning the value of anything except a local var, we must create a reference. When creating a reference, we upref the value instantly and have a deref token that can be disable by a move. Just like function return values.

## Detect circular refs

All objects of classes that can have circular refs are allocated in a separate block of memory. You loop over each object and for each property they have that has a type containing a circular ref class, you do: ob->prop->_RC_CHECK++. Then you loop all objects again, and if _RC != _RC_CHECK, you mark it as keep and also mark their properties as keep (recursive). Then loop a 3rd time and free all objects that arent marked as keep and where _RC == _RC_CHECK

On object create:
- Check if GC inited
- Add object to the list of the class type
- set RC_CHECK to 0

@gc:
- Check if GC inited
- Loop object list of each class
-- Each circular property, do: RC_CHECK++
-- Mark each object where RC_CHECK != RC as keep and also keep their properties recursively
-- Delete all unmarked objects

Compiler:
- generate GC[CLASS] class for each class
- generate globals foreach GC[CLASS] class: GC_{class gname}_{build->unique_id}
- generate init func:
-- Initialize all the GC[CLASS] classes and store them in the globals


## Circular refs Method 2

Add all circular objects to a list (shared over threads) when allocating the object. Start a new thread at the start of the program that loops the list every x seconds.
The loop:
- disable free'ing of circular objects
- sleep 1 ms
- loop each object
-- ref_check++ on each circular property recursively
...
