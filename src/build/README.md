
# Build

## Flow

```
-- STAGE 1
1 : Scan outer definitions/tokens: class, fn, use, header, extend, enum, ...
-- STAGE 2
2.1 : Read aliasses
2.2 : Read class/struct properties + types
2.3 : Detect circular classes/structs + determine class/struct sizes
2.4 : Update property types with size & set shared to `false` if not circular
2.5 : Read extend functions
2.6 : Read all types: functions, globals, enums
-- STAGE 3
3 : Read default values from class properties, function args, enums
-- STAGE 4
4.1 : Read function ASTs
4.2 : Generate IR
-- STAGE 5
5.1 : Generate o-files
5.2 : Link
```
