#include "SelfOrganizingMap.h"
#include "csvFile.h"
#include <chrono>
#include <cstdlib>
#include <iostream>

using namespace SOM;
using namespace csv;
using namespace std;

auto printNeuron = [](auto& _SOM)
{
	for (auto&& _Vector : _SOM.GetNeuron())
	{
		copy(_Vector.begin(), _Vector.end(), ostream_iterator<double>(cout, " "));
		cout << endl;
	}
	cout << endl;
};

int main()
{
	csv::FileReader _FileReader("Test.csv");
	auto _Data = _FileReader.Convert<std::vector<std::vector<double>>>(_FileReader.Row(), _FileReader.Column());
	if (0 == _Data.size())
	{
		return EXIT_FAILURE;
	}

	size_t _NeuronCount = 2;
	size_t _Iterator = 1000;
	double _Learning = 0.8;
	double _Neighborhood = 0;

	SelfOrganizingMap<double> _MySOM(_Data, _NeuronCount, _Iterator, _Learning, _Neighborhood, random_device()());

	while (_MySOM.Update())
	{
		auto _Iterator = _MySOM.GetIterator();
		if (0 == _Iterator % 1000)
		{
			cout << "Iterator : " << _Iterator << endl;
			printNeuron(_MySOM);
		}
	}

	cout << "Iterator : " << _MySOM.GetIterator() << endl;
	printNeuron(_MySOM);

	return EXIT_SUCCESS;
}
