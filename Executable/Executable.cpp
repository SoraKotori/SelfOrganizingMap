#include "SelfOrganizingMap.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

#include <fstream>
#include <sstream>
#include <cstring>
#include <utility>

#include <iostream>
#include <filesystem>

using namespace std;
using namespace SOM;
using namespace std::experimental::filesystem;

template<typename _Ty, typename _Size>
void csvParameter(path _Directories, _Ty _Learning, _Ty _Neighborhood, _Size _Dimension, _Size _kCluster, _Size _Iteration)
{
	ofstream _FileParameter(_Directories / "Parameter.csv");
	_FileParameter << "Learning: " << _Learning << endl;
	_FileParameter << "Neighborhood: " << _Neighborhood << endl;
	_FileParameter << "Dimension: " << _Dimension << endl;
	_FileParameter << "kCluster: " << _kCluster << endl;
	_FileParameter << "Iteration: " << _Iteration << endl;
}

template<typename _Ty, typename _Size>
auto csvDistance(path _Directories, const vector<_Ty>& _Center, const vector<_Ty>& _Data, const vector<_Size>& _DataCluster)
{
	auto _DataCount = _DataCluster.size();
	auto _Dimension = _Data.size() / _DataCount;
	auto _CenterCount = _Center.size() / _Dimension;

	vector<_Size> _Count(_CenterCount, 0);
	vector<_Ty> _Distance(_CenterCount, 0);

	for (decltype(_DataCount) _DataIndex = 0; _DataIndex < _DataCount; ++_DataIndex)
	{
		auto _DataClusterIndex = _DataCluster[_DataIndex];
		auto _DataIndexFirst = &_Data[_DataIndex];
		auto _CenterIndexFirst = &_Center[_DataClusterIndex];

		_Count[_DataClusterIndex]++;
		_Distance[_DataClusterIndex] +=
			EuclideanDistance(_DataIndexFirst, _DataIndexFirst + _Dimension, _CenterIndexFirst, _Ty(0));
	}

	decltype(auto) _First = _Count.begin();
	decltype(auto) _Last = _Count.end();
	decltype(auto) _Result = _Distance.begin();

	for (; _First != _Last; ++_First, ++_Result)
	{
		*_Result /= *_First;
	}

	ofstream _FileDistance(_Directories / "Distance.csv");

	_FileDistance << "id" << endl;
	for (decltype(_CenterCount) _CenterIndex = 0; _CenterIndex < _CenterCount; _CenterIndex++)
	{
		_FileDistance << _CenterIndex + 1;
		_FileDistance << ',' << _Distance[_CenterIndex];
		_FileDistance << endl;;
	}

	return move(_Distance);
}

class csv_ifstream : virtual public ifstream
{
public:
	template<typename... _Args>
	explicit csv_ifstream(_Args&&... __args) : ifstream(forward<_Args>(__args)...)
	{
		struct csv_type : ctype<char>
		{
			static const mask* get_table()
			{
				mask* __csv_table = new ctype::mask[table_size];
				memcpy(__csv_table, classic_table(), sizeof(mask) * table_size);

				__csv_table[','] |= space;
				return __csv_table;
			}

			csv_type(size_t __refs = 0U) : ctype(get_table(), true, __refs)
			{ }
		};

		imbue(locale(getloc(), new csv_type));
	}
};

template<typename _FileName, typename _Ty, typename _Size>
bool csv_read(_FileName&& __filename, vector<_Ty>& __data, _Size& __dimension, _Size& __data_count)
{
	typedef typename vector<_Ty>::size_type size_type;

	csv_ifstream __file(__filename);
	if (!__file)
	{
		return false;
	}

	size_type __comma = 0;
	size_type __line = 0;

	for (auto __char = __file.get(); EOF != __char; __char = __file.get())
	{
		switch (__char)
		{
		case ',':
			__comma++;
			break;
		case '\n':
			__line++;
			break;
		}
	}

	if (0 == __comma || 0 == __line)
	{
		return false;
	}

	__file.clear();
	__file.seekg(0, __file.beg);

	string _FirstLine;
	getline(__file, _FirstLine);

	size_type __fdimension = __comma / __line;
	size_type __fdata_count = __line - 1;
	vector<_Ty> __fdata(__fdimension * __fdata_count);

	auto __it_fdata = __fdata.begin();
	for (size_type __fdata_index = 0; __fdata_index < __fdata_count; __fdata_index++)
	{
		_Ty __data_id;
		__file >> __data_id;

		__it_fdata = copy_n(istream_iterator<_Ty>(__file), __fdimension, __it_fdata);
	}

	__data = move(__fdata);
	__dimension = static_cast<_Size>(__fdimension);
	__data_count = static_cast<_Size>(__fdata_count);

	return true;
}

template<typename _Ty, typename _Size>
bool csv_write(
	path _Directories,
	const vector<_Ty>& _Center,
	const vector<_Ty>& _Data,
	const vector<_Size>& _DataCluster)
{
	typedef typename vector<_Ty>::size_type size_type;
	auto _DataCount = static_cast<size_type>(_DataCluster.size());
	auto _Dimension = _Data.size() / _DataCount;
	auto _ClusterCount = _Center.size() / _Dimension;

	ofstream _FileCenter(_Directories / "Center.csv");
	if (!_FileCenter)
	{
		return false;
	}

	ofstream _FileOutput(_Directories / "Data.csv");
	if (!_FileOutput)
	{
		return false;
	}

	vector<ofstream> _FileCluster(_ClusterCount);
	for (decltype(_ClusterCount) _ClusterIndex = 0; _ClusterIndex < _ClusterCount; _ClusterIndex++)
	{
		ofstream _FileClusterIndex(_Directories / (to_string(_ClusterIndex) + ".csv"));
		if (!_FileClusterIndex)
		{
			return false;
		}

		_FileCluster[_ClusterIndex] = move(_FileClusterIndex);
	}

	_FileCenter << "id";
	_FileOutput << "id";
	for (size_type _DimensionIndex = 0; _DimensionIndex < _Dimension; _DimensionIndex++)
	{
		auto _FirstLine = string(",dim") + to_string(_DimensionIndex + 1);

		_FileCenter << _FirstLine;
		_FileOutput << _FirstLine;
	}
	_FileCenter << endl;
	_FileOutput << ',' << "cid" << endl;;

	auto _CenterFirst = _Center.begin();
	for (decltype(_ClusterCount) _ClusterIndex = 0; _ClusterIndex < _ClusterCount; _ClusterIndex++)
	{
		decltype(auto) _FileClusterIndex = _FileCluster[_ClusterIndex];

		_FileCenter << _ClusterIndex + 1;
		_FileClusterIndex << "id";
		for (size_type _DimensionIndex = 0; _DimensionIndex < _Dimension; _DimensionIndex++)
		{
			_FileCenter << ',' << *_CenterFirst++;
			_FileClusterIndex << ',' << "dim" << _DimensionIndex + 1;
		}
		_FileCenter << endl;;
		_FileClusterIndex << ',' << "cid" << endl;;
	}

	auto _DataFirst = _Data.begin();
	for (size_type _DataIndex = 0; _DataIndex < _DataCount; _DataIndex++)
	{
		decltype(auto) _DataClusterIndex = _DataCluster[_DataIndex];
		decltype(auto) _FileClusterIndex = _FileCluster[_DataClusterIndex];

		auto _DataIndexAdd = _DataIndex + 1;
		_FileOutput << _DataIndexAdd;
		_FileClusterIndex << _DataIndexAdd;
		for (size_type _DimensionIndex = 0; _DimensionIndex < _Dimension; _DimensionIndex++)
		{
			auto _DataValue = *_DataFirst++;

			_FileOutput << ',' << _DataValue;
			_FileClusterIndex << ',' << _DataValue;
		}
		_FileOutput << ',' << _DataClusterIndex << endl;
		_FileClusterIndex << ',' << _DataClusterIndex << endl;
	}

	return true;
}

template<typename _Type>
decltype(auto) VectorTo2DVector(vector<_Type> _Vector, typename vector<_Type>::size_type _Dimension)
{
	auto&& _Count = _Vector.size() / _Dimension;
	auto&& _Fires = begin(_Vector);
	vector<vector<_Type>> _Vector2D(_Count, vector<_Type>(_Dimension));
	for_each(begin(_Vector2D), end(_Vector2D), [&](auto&& _Vector)
	{
		copy_n(_Fires, _Dimension, begin(_Vector));
		_Fires = next(_Fires, _Dimension);
	});

	return _Vector2D;
}

template<typename _Type>
void Normalization(vector<vector<_Type>>& _2DVector)
{
	vector<_Type> _Min(_2DVector[0]);
	vector<_Type> _Max_Range(_2DVector[0]);

	auto&& _DimensionCount = _2DVector[0].size();
	for_each(next(begin(_2DVector)), end(_2DVector), [&, _DimensionCount](auto&& _Vector)
	{
		for (decltype(_DimensionCount) _Dimension = 0; _Dimension < _DimensionCount; ++_Dimension)
		{
			auto&& _Value = _Vector[_Dimension];
			auto&& _MinValue = _Min[_Dimension];
			auto&& _MaxValue = _Max_Range[_Dimension];

			if (_MinValue > _Value)
			{
				_MinValue = _Value;
			}
			else if (_MaxValue < _Value)
			{
				_MaxValue = _Value;
			}
		}
	});

	for (decltype(_DimensionCount) _Dimension = 0; _Dimension < _DimensionCount; ++_Dimension)
	{
		_Max_Range[_Dimension] -= _Min[_Dimension];
	}

	for (auto&& _Vector : _2DVector)
	{
		for (decltype(_DimensionCount) _Dimension = 0; _Dimension < _DimensionCount; ++_Dimension)
		{
			auto&& _Value = _Vector[_Dimension];
			_Value = (_Value - _Min[_Dimension]) / _Max_Range[_Dimension];
		}
	}
}

auto printNeuron = [](auto& _SOM)
{
	for (auto&& _Vector : _SOM.GetNeuron())
	{
		copy(_Vector.begin(), _Vector.end(), ostream_iterator<double>(cout, " "));
		cout << endl;
	}
	cout << endl;
};

template<typename _Tyep, typename _Size>
bool SOMTest(string _FileName, _Size _kCluster, _Tyep _Learning, _Tyep _Neighborhood, _Size _IteratorCount)
{
	vector<double> _Data;
	int _Dimension;
	int _DataSize;

	bool _bResult = csv_read(_FileName, _Data, _Dimension, _DataSize);
	if (false == _bResult)
	{
		return false;
	}

	auto&& _Data2D = VectorTo2DVector(_Data, _Dimension);
	Normalization(_Data2D);

	SelfOrganizingMap<double> _MySOM(_Data2D, _kCluster, _IteratorCount, _Learning, _Neighborhood, random_device()());

	int _Iteration;
	for (_Iteration = 0; _Iteration < _IteratorCount; _Iteration++)
	{
		if (!_MySOM.Update())
		{
			break;
		}

		if (0 == _Iteration % 100)
		{
			cout << "Iteration:" << _Iteration << endl;
		}
	}

	printNeuron(_MySOM);

	vector<size_t> _DataCluster(_DataSize);
	for (int Index = 0; Index < _DataSize; Index++)
	{
		_DataCluster[Index] = _MySOM.GetDataCluster(_Data2D[Index]);
	}

	auto&& _Center2D = _MySOM.GetNeuron();
	vector<double> _Center;
	for_each(begin(_Center2D), end(_Center2D), [&_Center](auto&& _Vector)
	{
		_Center.insert(end(_Center), begin(_Vector), end(_Vector));
	});

	string _FileString(_FileName);
	_FileString.erase(_FileString.find('.'));
	path _Directories(_FileString);
	_Directories /= string("Cluster_") + to_string(_kCluster);
	_Directories /= string("_Learning_") + to_string(_Learning);
	_Directories /= string("_Neighborhood_") + to_string(_Neighborhood);
	create_directories(_Directories);

	_bResult = csv_write(_Directories, _Center, _Data, _DataCluster);
	if (false == _bResult)
	{
		return false;
	}

	csvDistance(_Directories, _Center, _Data, _DataCluster);
	csvParameter(_Directories, _Learning, _Neighborhood, _Dimension, _kCluster, _Iteration);
	return true;
}

int main()
{
	vector<string> _FileName = { "Dubai3.csv_d3.csv", "Ellipse_300_3C2D_Or.csv", "flower.csv_d3.csv", "Line_450_2C2D_Clean_Or.csv" };
	vector<int> _kCluster = { 3, 2, 3, 2 };
	vector<double> _Learning = { 0.2, 0.5 ,0.8 };
	vector<double> _Neighborhood = { 0.1, 0.2 ,0.3 };
	auto _IteratorCount = 10000;

	auto&& _Count = _FileName.size();
	for (decltype(_Count) Index = 0; Index < _Count; ++Index)
	{
		for (auto&& _LearningValue : _Learning)
		{
			for (auto&& _NeighborhoodValue : _Neighborhood)
			{
				SOMTest(_FileName[Index], _kCluster[Index], _LearningValue, _NeighborhoodValue, _IteratorCount);
			}
		}
	}

	return EXIT_SUCCESS;
}
