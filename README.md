# tinyXML
A C++ library of XML serializer and de-serializer using functional programming, template partial specialization and reflection.

It can serialize C++ objects such as int, arrays, strings, vectors, maps and pointers into binary representations or XML representations. It also supports de-serialization.



## Usage

```c++
#include "bin_srl.h"
#include "xml_srl.h"
#include "type_info.h"
#include "tinyxml2.h"
```



Binary serialization and de-serialization:

```C++
bin_srl::serialize(a, "output.bin");
bin_srl::deserialize(b, "input.bin");
```



XML serialization and de-serialization:

```C++
xml_srl::serialize(a, "output.bin");
xml_srl::deserialize(b, "input.bin");
```

