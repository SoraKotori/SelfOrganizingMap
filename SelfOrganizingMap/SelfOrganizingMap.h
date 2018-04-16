#pragma once
#pragma once
#include <algorithm>
#include <functional>
#include <iterator>
#include <random>
#include <vector>
#include <numeric>
#include <type_traits>

namespace SOM
{
	using namespace std;

	template<typename _InputIterator1, typename _InputIterator2, typename _Type>
	inline _Type SumSquares(_InputIterator1 _First1, _InputIterator1 _Last1, _InputIterator2 _First2, _Type _Init)
	{
		using namespace std::placeholders;
		return inner_product(_First1, _Last1, _First2, _Init, plus<>(), bind(pow<_Type, int>, bind(minus<>(), _1, _2), 2));
	}

	template<typename... _Args>
	inline decltype(auto) EuclideanDistance(_Args&&... __args)
	{
		return std::sqrt(SumSquares(forward<_Args>(__args)...));
	}

	template<typename _ForwardIterator1, typename _ForwardIterator2>
	inline decltype(auto) MinDistance(_ForwardIterator1 __first1, _ForwardIterator1 __last1,
		_ForwardIterator2 __first2, _ForwardIterator2 __last2)
	{
		typedef typename _ForwardIterator2::value_type value_type;

		auto _MinIterator = __first1;
		if (__first1 != __last1)
		{
			auto&& _MinDistance = SumSquares(__first2, __last2, __first1->begin(), value_type(0));
			while (++__first1 != __last1)
			{
				auto&& _Distance = SumSquares(__first2, __last2, __first1->begin(), value_type(0));
				if (_MinDistance > _Distance)
				{
					_MinDistance = _Distance;
					_MinIterator = __first1;
				}
			}
		}
		return _MinIterator;
	}

	template<typename _InputIterator, typename _OutputIterator, typename _Type>
	inline void UpdateWeight(_InputIterator __first, _InputIterator __last,
		_OutputIterator __result, _Type _Kernel)
	{
		for (; __first != __last; ++__first, ++__result)
		{
			*__result += _Kernel * (*__first - *__result);
		}
	}

	template<typename _Type, typename _EngineType = default_random_engine>
	class SelfOrganizingMap
	{
	public:
		using _VectorType = vector<_Type>;
		typedef typename vector<vector<_Type>>::size_type size_type;

		SelfOrganizingMap() = default;
		~SelfOrganizingMap() = default;

		template<typename... _Args>
		SelfOrganizingMap(const vector<vector<_Type>>& __input, size_type _NeuronCount, size_type _Iterator,
			_Type _Learning, _Type _Neighborhood, _Args&&... __args
		) :
			_IteratorCount(_Type(_Iterator)),
			_LearningAlpha(_Learning),
			_NeighborhoodSigma(_Neighborhood),
			_InputSequence(__input.begin(), __input.end()),
			_Neuron(_NeuronCount, vector<_Type>(__input.begin()->size())),
			_Engine(forward<_Args>(__args)...)
		{
			Reset();
		}

		void Reset()
		{
			for (auto&& _Vector : _Neuron)
			{
				generate(_Vector.begin(), _Vector.end(), [this]
				{ return generate_canonical<_Type, numeric_limits<_Type>::digits>(_Engine); });
			}
		}

		bool Update()
		{
			auto&& _Learning = Exponential(_LearningAlpha);
			auto&& _Sigma = Exponential(_NeighborhoodSigma);
			auto&& _Neighbourhood = _Sigma * _Sigma;
			auto&& _2Neighbourhood = _Type(2) * _Neighbourhood;

			shuffle(_InputSequence.begin(), _InputSequence.end(), _Engine);
			for (auto&& _InputVector : _InputSequence)
			{
				auto&& _InputFirst = _InputVector.get().begin();
				auto&& _InputLast = _InputVector.get().end();

				auto&& _NeuronFirst = _Neuron.begin();
				auto&& _NeuronLast = _Neuron.end();
				auto&& _NeuronMidden = MinDistance(_NeuronFirst, _NeuronLast, _InputFirst, _InputLast);

				auto&& _BestNeuronFirst = _NeuronMidden->begin();
				auto&& _BestNeuronLast = _NeuronMidden->end();

				auto&& UpdateNeuron = [_InputFirst, _InputLast](auto&& _NeuronFirst, auto&& _Kernel)
				{ return UpdateWeight(_InputFirst, _InputLast, _NeuronFirst, _Kernel); };

				for (; _NeuronFirst != _NeuronMidden; ++_NeuronFirst)
				{
					auto&& _ThisNeuronFirst = _NeuronFirst->begin();
					auto&& _Distance = SumSquares(_BestNeuronFirst, _BestNeuronLast, _ThisNeuronFirst, _Type(0));

					if (_Neighbourhood > _Distance)
					{
						auto&& _Kernel = _Learning * exp(-_Distance / _2Neighbourhood);
						UpdateNeuron(_ThisNeuronFirst, _Kernel);
					}
				}

				while (++_NeuronFirst != _NeuronLast)
				{
					auto&& _ThisNeuronFirst = _NeuronFirst->begin();
					auto&& _Distance = SumSquares(_BestNeuronFirst, _BestNeuronLast, _ThisNeuronFirst, _Type(0));

					if (_Neighbourhood > _Distance)
					{
						auto&& _Kernel = _Learning * exp(-_Distance / _2Neighbourhood);
						UpdateNeuron(_ThisNeuronFirst, _Kernel);
					}
				}

				UpdateNeuron(_BestNeuronFirst, _Learning);
			}

			return _IteratorCount != ++_Iterator;
		}

		decltype(auto) GetNeuron()
		{
			return _Neuron;
		}

		decltype(auto) GetIterator()
		{
			return size_type(_Iterator);
		}

		decltype(auto) GetDataCluster(const _VectorType& _Vector)
		{
			auto&& _Last = MinDistance(begin(_Neuron), end(_Neuron), begin(_Vector), end(_Vector));
			return distance(begin(_Neuron), _Last);
		}

	private:
		_Type _Iterator = _Type(0);
		_Type _IteratorCount = _Type(0);
		_Type _LearningAlpha = _Type(0);
		_Type _NeighborhoodSigma = _Type(0);

		vector<reference_wrapper<const vector<_Type>>> _InputSequence;
		vector<vector<_Type>> _Neuron;

		_EngineType _Engine;

		decltype(auto) Linear(const _Type& _Initial)
		{
			return ((-_Iterator * _Initial) / _IteratorCount + _Initial);
		}

		decltype(auto) Exponential(const _Type& _Initial)
		{
			return (_Initial * exp(-_Iterator / _IteratorCount));
		}
	};
}