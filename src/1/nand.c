#include "nand.h"

#include <stdlib.h>
#include <errno.h>

#define max(a, b) ((a) > (b) ? (a) : (b))

// circuit >>
typedef struct block block_t;

size_t COUNT = 0; // global number of all NANDs
block_t** CIRCUIT = NULL; // global array allocated during nand_evaluate()
const size_t DEFAULT_INITIAL_ARRAY_CAPACITY = 4;

struct block { // structure to "cache" temporary results and detect cycles during nand_evaluate()
	bool cycle;
	bool signal;
	int path;
};

static void circuit_init() {
	CIRCUIT = (block_t**)malloc(COUNT * sizeof(block_t*));
	if (!CIRCUIT) {
		errno = ENOMEM;
		return;
	}

	for (size_t i = 0; i < COUNT; ++i) {
		CIRCUIT[i] = NULL;
	}
}

static void circuit_clear() {
	for (size_t i = 0; i < COUNT; ++i) {
		free(CIRCUIT[i]);
        CIRCUIT[i] = NULL;
	}

	free(CIRCUIT);
	CIRCUIT = NULL;
}

static bool is_in_current_cycle(size_t id) {
	return CIRCUIT[id]->cycle;
}

static bool is_visited(size_t id) {
	return CIRCUIT[id] != NULL;
}

static block_t* new_block() {
	block_t* b = (block_t*)malloc(sizeof(block_t));
	if (!b) {
		errno = ENOMEM;
		return NULL;
	}

	b->cycle = true;
	b->signal = true;
	b->path = -1;

	return b;
}

static void visit(size_t id) {
	CIRCUIT[id] = new_block();
}

static void leave(size_t id) {
	CIRCUIT[id]->cycle = false;
}

static bool get_signal(size_t id) {
	return CIRCUIT[id]->signal;
}

static void set_signal(size_t id, bool s) {
	CIRCUIT[id]->signal = s;
}

static int get_path(size_t id) {
	return CIRCUIT[id]->path;
}

static void set_path(size_t id, int p) {
	CIRCUIT[id]->path = p;
}

// circuit <<

// list >>

typedef struct list {
    size_t size;
    size_t capacity;
    nand_t** array;
} list_t;

static list_t* new_list() {
	list_t* l = (list_t*)malloc(sizeof(list_t));
	if (!l) {
		errno = ENOMEM;
		return NULL;
	}

	l->size = 0;
    l->capacity = 0;
    l->array = NULL;

	return l;
}

static void init_list(list_t* l) {
    l->array = (nand_t**)malloc(DEFAULT_INITIAL_ARRAY_CAPACITY * sizeof(nand_t*));
    l->capacity = DEFAULT_INITIAL_ARRAY_CAPACITY;
    for (size_t i = 0; i < l->capacity; ++i) {
        l->array[i] = NULL;
    }

    if (!l->array) {
        errno = ENOMEM;
        return;
    }

    l->size = 0;
}

static void resize_list(list_t* l) {
    l->array = (nand_t**)realloc(l->array, 2 * l->capacity * sizeof(nand_t*));
    l->capacity *= 2;
    for (size_t i = l->capacity / 2; i < l->capacity; ++i) {
        l->array[i] = NULL;
    }

    if (!l->array) {
        errno = ENOMEM;
        return;
    }
}

static void push_back(list_t* l, nand_t* g) {
    if (!l || !g) {
        errno = EINVAL;
        return;
    }

	if (l->capacity == 0) {
        init_list(l);
	}
    if (l->size == l->capacity) {
        resize_list(l);
    }
    l->array[l->size] = g;
    ++(l->size);
}

static void remove_at_index(list_t* l, size_t i) {
    if (!l || i >= l->size) {
        errno = EINVAL;
        return;
    }

    l->array[i] = l->array[l->size - 1];
    l->array[l->size - 1] = NULL;
    --(l->size);
}

static bool remove_at_value(list_t* l, nand_t* g) { // returns true if the gate was removed, false otherwise
    if (!l || !l->array || !g) {
        errno = EINVAL;
        return false;
    }

    if (l->size == 0) {
        return false;
    }

    for (size_t i = 0; i < l->size; ++i) {
        if (l->array[i] == g) {
            remove_at_index(l, i);
            return true;
        }
    }

    return false;
}

static void clear_list(list_t* l) {
    free(l->array);
    l->array = NULL;
    l->size = 0;
    l->capacity = 0;

    l = NULL;
}

// list <<

// input >>

typedef union data { // NAND input data - signal or NAND gate
	bool* signal;
	nand_t* gate;
} data_t;

typedef struct input {
	bool is_nand;
	data_t connection;
} input_t;

static input_t* new_input_nand(nand_t* g) {
	if (!g) {
		errno = EINVAL;
		return NULL;
	}

	input_t* in = (input_t*)malloc(sizeof(input_t));
	if (!in) {
		errno = ENOMEM;
		return NULL;
	}

	in->is_nand = true;
	in->connection.gate = g;

	return in;
}

static input_t* new_input_signal(const bool* s) {
	if (!s) {
		errno = EINVAL;
		return NULL;
	}

	input_t* in = (input_t*)malloc(sizeof(input_t));
	if (!in) {
		errno = ENOMEM;
		return NULL;
	}

	in->is_nand = false;
	in->connection.signal = (bool*)s;

	return in;
}

// input <<

// obligatory >>

struct nand {
	size_t input_size;
    size_t id;
	input_t** input_ports;
	list_t* output_ports;
};

static void set_id(nand_t* g) {
	g->id = COUNT;
	++COUNT;
}

nand_t* nand_new(unsigned n) {
	nand_t* g = (nand_t*)malloc(sizeof(nand_t));
	if (!g) {
		errno = ENOMEM;
		return NULL;
	}

	g->input_size = (size_t)n;
	g->input_ports = (input_t**)malloc(n * sizeof(input_t*));
	if (!g->input_ports) {
		errno = ENOMEM;
		return NULL;
	}
	for (unsigned i = 0; i < n; ++i) {
		g->input_ports[i] = NULL;
	}

	g->output_ports = new_list();
	if (!g->output_ports) {
		errno = ENOMEM;
		return NULL;
	}

	set_id(g);

	return g;
}

static void disconnect_all_outputs_to_the_gate(nand_t* gate_to_delete, nand_t* g_out) {
    if (!gate_to_delete || !g_out) {
        errno = EINVAL;
        return;
    }

    while (remove_at_value(g_out->output_ports, gate_to_delete));
}

static void disconnect_all_inputs_from_the_gate(nand_t* gate_to_delete, nand_t* g_in) {
    if (!gate_to_delete || !g_in) {
        errno = EINVAL;
        return;
    }

    for (size_t i = 0; i < g_in->input_size; ++i) {
        if (!g_in->input_ports[i] || !g_in->input_ports[i]->is_nand) {
            continue;
        }
        if (g_in->input_ports[i]->connection.gate == gate_to_delete) {
            free(g_in->input_ports[i]);
            g_in->input_ports[i] = NULL;
        }
    }
}

void nand_delete(nand_t* g) {
	if (!g) {
		return;
	}

	for (size_t i = 0; i < g->input_size; ++i) {
        if (!g->input_ports[i]) {
            continue;
        }

        if (g->input_ports[i]->is_nand) {
			disconnect_all_outputs_to_the_gate(g, g->input_ports[i]->connection.gate);
        }

		free(g->input_ports[i]);
        g->input_ports[i] = NULL;
	}
	free(g->input_ports);
    g->input_ports = NULL;

    for (size_t i = 0; i < g->output_ports->size; ++i) {
        disconnect_all_inputs_from_the_gate(g, g->output_ports->array[i]);
    }

	clear_list(g->output_ports);
	free(g->output_ports);
    g->output_ports = NULL;

    free(g);
}

static void disconnect_input(nand_t* g_in, size_t k) { // disconnects gate or signal from port k in gate g_in
	if (!g_in->input_ports[k]) {
		return;
	}
	if (g_in->input_ports[k]->is_nand) {
        remove_at_value(g_in->input_ports[k]->connection.gate->output_ports, g_in);
	}

    free(g_in->input_ports[k]);
	g_in->input_ports[k] = NULL;
}

int nand_connect_nand(nand_t* g_out, nand_t* g_in, unsigned k) { // connects two NAND gates
	if (!g_out || !g_in || k >= g_in->input_size) {
		errno = EINVAL;
		return -1;
	}

	disconnect_input(g_in, k);

	g_in->input_ports[k] = new_input_nand(g_out);
	push_back(g_out->output_ports, g_in);

	return 0;
}

int nand_connect_signal(bool const* s, nand_t* g, unsigned k) { // connects signal to NAND gate
	if (!g || !s || k >= g->input_size) {
		errno = EINVAL;
		return -1;
	}

	disconnect_input(g, k);
	g->input_ports[k] = new_input_signal(s);

	return 0;
}

static void evaluate(input_t* in, bool* signal, int* path) { // auxiliary function for get_path_and_set_signal()
	if (!in) {
		errno = EINVAL;
		return;
	}
	if (!in->is_nand) {
		*signal = *in->connection.signal;
		*path = 0;
		return;
	}

	nand_t* g = in->connection.gate;
	if (is_visited(g->id)) {
		if (is_in_current_cycle(g->id)) {
			errno = ECANCELED;
			return;
		}

		*signal = get_signal(g->id);
		*path = get_path(g->id);
		return;
	}

	visit(g->id);

	bool temp_signal = false;
	set_signal(g->id, false);
	int temp_path;
	int res_path = -1;

	for (unsigned i = 0; i < g->input_size; ++i) {
		if (!g->input_ports[i]) {
			errno = ECANCELED;
			return;
		}

		evaluate(g->input_ports[i], &temp_signal, &temp_path);

		if (temp_signal == false) {
			set_signal(g->id, true);
		}

		res_path = max(res_path, temp_path);
	}

	*signal = get_signal(g->id);
	set_path(g->id, 1 + res_path);
	*path = 1 + res_path;

	leave(g->id);
}

static int get_path_and_set_signal(nand_t** gates, bool* signals, size_t i) { // auxiliary function for nand_evaluate()
	bool signal;
	int path;

	if (!gates[i]) {
		errno = EINVAL;
		return -1;
	}

	input_t* in = new_input_nand(gates[i]);
	evaluate(in, &signal, &path);
	free(in);
	if (errno) {
		return -1;
	}

	signals[i] = signal;
	return path;
}

ssize_t nand_evaluate(nand_t** g, bool* s, size_t m) { // evaluates longest critical path and sets signals
	if (!g || !s || !m) {
		errno = EINVAL;
		return -1;
	}
	
	for (size_t i = 0; i < m; ++i) {
		if (!g[i]) {
			errno = EINVAL;
			return -1;
		}
	}

	circuit_init();

	ssize_t path = -1;
	for (size_t i = 0; i < m; ++i) {
		path = max(path, get_path_and_set_signal(g, s, i));
		if (errno) {
			break;
		}
	}

	circuit_clear();

	if (errno) {
		return -1;
	}

	return path;
}

ssize_t nand_fan_out(nand_t const* g) { // gets outputs count of gate g
	if (!g) {
		errno = EINVAL;
		return -1;
	}

	return g->output_ports->size;
}

void* nand_input(nand_t const* g, unsigned k) { // gets input connected to input port k in gate g
	if (!g || k >= g->input_size) {
		errno = EINVAL;
		return NULL;
	}

	if (!g->input_ports[k]) {
		errno = 0;
		return NULL;
	}

	return g->input_ports[k]->is_nand 
		? (void*)g->input_ports[k]->connection.gate
		: (void*)g->input_ports[k]->connection.signal;
}

nand_t* nand_output(nand_t const* g,ssize_t k) { // gets output connected to output port k in gate g
    if (k < 0 || (size_t)k >= g->output_ports->size) {
        errno = EINVAL;
        return NULL;
    }
    
    return g->output_ports->array[k];
}
