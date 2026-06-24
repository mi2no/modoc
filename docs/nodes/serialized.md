Every node provides a serialized json representation. The feature is used
by the `@gen` node.

There is no strict struture - it is defined by the node itself.
The only standardized fields are:
- type [string, required] - the node type
- tags [string list, optional] - tags

Example:
```
{
    "type": "sec";
    "tags": ["tag1", "tag2"];
    "title": "Section";
    "children": [
        {
            "type": "sec";
            "title": "Subsection";
        },
        {
            "type": "text";
            "content": "Hello world!";
        }
    ]
}
```

