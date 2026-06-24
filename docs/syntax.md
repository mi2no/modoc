## Nodes
modoc is a node-based language. To define a node use `@`, as follows:
```
@node
```

### Scopes
Scopes are based on indentation (like in python):
```
@node
    This is the node's scope.
    Still the same scope.
The scope ends here.
```

### Options
A node can take in options in the form of a list:
```
@node[opt1 = 1, opt2 = "Hello", opt3 = [1, 2, 3]]
```
This is optional.

### Attributes
A node can be assigned tags - for styling:
```
@node(tag1, tag2)
```
This is optional.

## Styling
modoc allows css-like node styling. The implementation of a style depends entirely on the backend.
To define the style use the special `@style` node at the top of your document. 
`@style` can only appear once in the document.
