## Overview
modoc doesn't have a built-in programmatic way of generating nodes. 
This is because modoc is supposed to be simple. An embedded language clashes with this take
and could never reach the capabilities of already established programming languages.
That is why computation is delegated to external tools.

The `@gen` node embeds a node subtree created from the result of a given command.
The command is a string option parameter:
```
@gen["./program"]
```

### Options
The options are (in order):

1. cmd [string, required] - the command to be executed
2. fd [num, optional] - explicit file descriptor

## Parsing a subtree
The executed command must output a serialized json node tree structure.
Example:
```
{
    "type": "sec"
    "title": "Section"
}
```

### Output
The serialized json structure must be outputed to a temporary file or file descriptor
defined by the environmental variables: `MODOC_JSON_FILE`, `MODOC_JSON_FD`. It is possible
to explicitly define a different file descriptor (for example stdout) by providing the fd
options parameter.

### JSON representation
```
{
    "type": "gen";
    "command": "./program";
}
```
