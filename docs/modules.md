# Modules

```ghl
import math;           // resolves to std/math.ghl
import "./utils.ghl";  // relative path
```

The compiler loads the module, parses it, and merges functions/structs into the current program.

Set `GHL_STD` to point at the `std/` directory if building from another working directory.
