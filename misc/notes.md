
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
