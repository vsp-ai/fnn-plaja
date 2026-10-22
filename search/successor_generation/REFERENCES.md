## References & Credits

This package adapts algorithmic methods from [PRISM](https://github.com/prismmodelchecker/prism)
at commit [8394df8e1a0058cec02f47b0437b185e3ae667d7](https://github.com/prismmodelchecker/prism/tree/8394df8e1a0058cec02f47b0437b185e3ae667d7) (March 20, 2019),
specifically in [action_op.cpp](action_op.cpp).

The [successor generation](successor_generator_c.h), which caches [action operators](action_op.h) at initialization time,
has been designed under the impression of PRISM's [explicit-state successor generation](https://github.com/prismmodelchecker/prism/tree/8394df8e1a0058cec02f47b0437b185e3ae667d7/prism/src/simulator).
