
set(nr_SRC
	nr-object.cpp
	nr-point-fns.cpp
	nr-values.cpp

	# -------
	# Headers
	nr-macros.h
	nr-object.h
	nr-point-fns.h
	nr-values.h
)

add_inkscape_lib(nr_LIB "${nr_SRC}")
