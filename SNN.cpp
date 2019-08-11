// SNN.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
// clib
#include <iostream>
#include <cmath>
#include <random>
#include <string>
#include <sstream>

// stl
#include <vector>

#define FPOINT				double
#define INPUT_LAYER_IDX		0 // always 0

// implements a neuron building block containing three values:
// the raw input value, the fast sigmoid result, and the derivative
// of a fast sigmoid
class neuron
{
public:

	neuron(FPOINT val)
	{
		this->raw = val;
		// activate via fast sigmoid
		this->calc_fs();
		// calculate fast sigmoid derivative
		this->calc_fsd();
	}
	// activation function
	// f(x) = x / (1 + abs(x))
	void calc_fs()
	{
		this->fs = this->raw / (1.0f + std::abs(this->raw));
		// 1 / std::abs(x) can never be 0 so no need to check on division
	}

	// derivative of fast sigmoid function
	// f'(x) = f(x) * (1 - f(x))
	void calc_fsd()
	{
		this->fsd = this->fs * (1.0f - this->fs);
	}

	// get/set
	FPOINT get_raw() { return this->raw; }
	FPOINT get_fs() { return this->fs; }
	FPOINT get_fsd() { return this->fsd; }
	void set_raw(FPOINT new_raw) 
	{
		this->raw = new_raw;
		this->calc_fs();
		this->calc_fsd();
	}
private:
	// raw
	FPOINT raw;
	// linearized
	FPOINT fs;
	// derivative
	FPOINT fsd;
};

class matrix
{
public:
	matrix(std::size_t nrow, std::size_t ncol)
	{
		for (std::size_t row_idx = 0; row_idx < nrow; row_idx++)
		{
			std::vector<FPOINT> column_matrix_storage; // inner
			for (std::size_t col_idx = 0; col_idx < ncol; col_idx++)
			{
				column_matrix_storage.push_back(0.0f);
			}
			this->matrix_storage.push_back(column_matrix_storage);
		}
	}

	~matrix()
	{
		// no dynamic allocations
	}

	void set_value(std::size_t row, std::size_t col, FPOINT value)
	{
		matrix_storage.at(row).at(col) = value;
	}

	FPOINT get_value(std::size_t row, std::size_t col)
	{
		return matrix_storage.at(row).at(col);
	}

	std::size_t get_row_size()
	{
		// assume first dimension is rows
		return (std::size_t)  matrix_storage.size();
	}

	std::size_t get_col_size()
	{
		// since vector of vectors, and all interior vectors
		// are identical in size, we can use the first one to get the
		// length of the second dimension
		return (std::size_t) matrix_storage.at(0).size();
	}

	// relies constructor has been used and valid
	void randomize()
	{
		std::random_device rdev;
		std::mt19937 rng(rdev());
		std::uniform_real_distribution<FPOINT> distribution(0.0f, 1.0f);
		std::size_t row_size = this->get_row_size();
		std::size_t col_size = this->get_col_size();

		for (std::size_t row_idx = 0; row_idx < row_size; row_idx++)
		{
			for (std::size_t col_idx = 0; col_idx < col_size; col_idx++)
			{
				this->matrix_storage.at(row_idx).at(col_idx) = distribution(rng);
			}
		}
	}

	// you must manually dispose of the object created by this function
	matrix* transpose(void)
	{
		matrix* transposed = new matrix(this->get_col_size(), this->get_row_size());
		std::size_t row_size = transposed->get_row_size();
		std::size_t col_size = transposed->get_col_size();
		for (std::size_t row_idx = 0; row_idx < row_size; row_idx++)
		{
			for (std::size_t col_idx = 0; col_idx < col_size; col_idx++)
			{
				transposed->set_value(row_idx, col_idx, this->get_value(col_idx, row_idx));
			}
		}

		return transposed;
	}

	std::string get_string()
	{
		std::stringstream sstream;
		std::size_t row_size = this->get_row_size();
		std::size_t col_size = this->get_col_size();

		for (std::size_t row_idx = 0; row_idx < row_size; row_idx++)
		{
			for (std::size_t col_idx = 0; col_idx < col_size; col_idx++)
			{
				sstream << this->matrix_storage.at(row_idx).at(col_idx)
					<< " \t";
			}
			sstream << "\r\n";
		}
		return sstream.str();
	}

private:
	// outer vector is rows, inner vector is columns
	std::vector<std::vector<FPOINT>> matrix_storage;
};

class layer
{
public:
	// constructs neurons in a layer
	layer(std::size_t size)
	{
		for (std::size_t idx = 0; idx < size; idx++)
		{
			// create and allocate the neurons
			neuron* n = new neuron(0.0f);
			this->neuron_storage.push_back(n);
		}
	}

	// destroys all neurons in a layer
	~layer()
	{
		for (std::size_t idx = 0; idx < this->neuron_storage.size(); idx++)
		{
			if (this->neuron_storage.at(idx) != nullptr)
			{
				delete this->neuron_storage.at(idx);
			}
		}
	}

	// assigns input raw value to neuron
	void set_value_at(std::size_t at, FPOINT val)
	{
		this->neuron_storage.at(at)->set_raw(val);
	}

	// creates a matrix representation of the neuron layer raw values
	matrix* raw_matrix_of()
	{
		matrix* m = new matrix(1 /*always one dimension*/, 
			this->neuron_storage.size());

		for (std::size_t idx = 0; idx < this->neuron_storage.size(); idx++)
		{
			m->set_value(0, idx, this->neuron_storage.at(idx)->get_raw());
		}
	}

	// fast sigmoid calculated values of neurons in layer
	matrix* fs_matrix_of()
	{
		matrix* m = new matrix(1 /*always one dimension*/,
			this->neuron_storage.size());

		for (std::size_t idx = 0; idx < this->neuron_storage.size(); idx++)
		{
			m->set_value(0, idx, this->neuron_storage.at(idx)->get_fs());
		}
	}

	// fast sigmoid derivative values of neurons in layer
	matrix* fsd_matrix_of()
	{
		matrix* m = new matrix(1 /*always one dimension*/,
			this->neuron_storage.size());

		for (std::size_t idx = 0; idx < this->neuron_storage.size(); idx++)
		{
			m->set_value(0, idx, this->neuron_storage.at(idx)->get_fsd());
		}
	}
private:
	std::vector<neuron*> neuron_storage;
};


class network
{
	typedef std::vector<std::size_t> topology_vector;
public:
	network(topology_vector topology)
	{
		this->topology = topology;
		for (std::size_t top_idx = 0; top_idx < topology.size(); top_idx++)
		{
			layer* l = new layer(topology.at(top_idx));
			this->layer_storage.push_back(l);
		}
		// there's topology size - 1 weight matrices
		for (std::size_t top_idx = 0; top_idx < topology.size() - 1; top_idx++)
		{
			// in the weight matrix number of rows is the number of input neurons
			// and the number of columns is the number of feed-into output neurons.
			matrix* m = new matrix(topology.at(top_idx), topology.at(top_idx + 1));
			m->randomize(); // initial values are random
			this->weight_matrices_storage.push_back(m);
		}
	}

	// destroy all dynamically allocated instances
	~network()
	{
		for (std::size_t layer_idx = 0; layer_idx < this->layer_storage .size(); layer_idx++)
		{
			if (this->layer_storage.at(layer_idx) != nullptr)
			{
				delete this->layer_storage.at(layer_idx);
			}
		}

		for (std::size_t wmatrix_idx = 0; wmatrix_idx < this->layer_storage.size(); wmatrix_idx++)
		{
			if (this->weight_matrices_storage.at(wmatrix_idx) != nullptr)
			{
				delete this->weight_matrices_storage.at(wmatrix_idx);
			}
		}
	}

	void set_input(std::vector<FPOINT> input)
	{
		for (std::size_t idx = 0; idx < input.size(); idx++)
		{
			this->layer_storage.at(INPUT_LAYER_IDX)->set_value_at(idx, input.at(idx));
		}
		this->last_input = input;
	}
private:

	topology_vector topology;
	std::vector<layer*> layer_storage;
	std::vector<matrix*> weight_matrices_storage;
	std::vector<FPOINT> last_input;
};

int main()
{
	// NEURON CREATED
	neuron *n = new neuron(1.5f);
	std::cout << "Neuron constructor test:" << std::endl;
	std::cout << "raw: " << n->get_raw() << std::endl;
	std::cout << "fs: " << n->get_fs() << std::endl;
	std::cout << "dfs: " << n->get_fsd() << std::endl << std::endl;

	// MATRIX CREATED
	matrix *m = new matrix(3, 2);
	std::cout << "Matrix constructor test:" << std::endl;
	std::cout << m->get_string() << std::endl;

	std::cout << "Matrix randomization test:" << std::endl;
	m->randomize();
	std::cout << m->get_string() << std::endl;

	std::cout << "Matrix transpose test:" << std::endl;
	matrix* mnew = m->transpose();
	std::cout << mnew->get_string() << std::endl;
}