/*******************************************************************************
* CGoGN: Combinatorial and Geometric modeling with Generic N-dimensional Maps  *
* Copyright (C) 2015, IGG Group, ICube, University of Strasbourg, France       *
*                                                                              *
* This library is free software; you can redistribute it and/or modify it      *
* under the terms of the GNU Lesser General Public License as published by the *
* Free Software Foundation; either version 2.1 of the License, or (at your     *
* option) any later version.                                                   *
*                                                                              *
* This library is distributed in the hope that it will be useful, but WITHOUT  *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or        *
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License  *
* for more details.                                                            *
*                                                                              *
* You should have received a copy of the GNU Lesser General Public License     *
* along with this library; if not, write to the Free Software Foundation,      *
* Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.           *
*                                                                              *
* Web site: http://cgogn.unistra.fr/                                           *
* Contact information: cgogn@unistra.fr                                        *
*                                                                              *
*******************************************************************************/

#ifndef CORE_BASIC_CELL_MARKER_H_
#define CORE_BASIC_CELL_MARKER_H_

#include <core/container/chunk_array.h>
#include <core/map/map_base_data.h>
#include <type_traits>

namespace cgogn
{

class CellMarkerGen
{
public:
	typedef CellMarkerGen Super;
	CellMarkerGen()
	{}

	virtual ~CellMarkerGen();

	CellMarkerGen(const Super& dm) = delete;
	CellMarkerGen(Super&& dm) = delete;
	CellMarkerGen& operator=(Super&& dm) = delete;
	CellMarkerGen& operator=(const Super& dm) = delete;
};

template <typename MAP, unsigned int ORBIT>
class CellMarkerT : public CellMarkerGen
{
	static_assert(ORBIT >= VERTEX1, "ORBIT must be greater than or equal to VERTEX1");
	static_assert(ORBIT <= VOLUME3, "ORBIT must be less than or equal to VOLUME3");
public:
	typedef CellMarkerGen Inherit;
	typedef CellMarkerT< MAP, ORBIT > Super;

	typedef std::integral_constant<unsigned int, ORBIT> Orbit;
	typedef MAP Map;
	typedef typename Map::ChunkSizeType ChunkSizeType;
	typedef ChunkArray<ChunkSizeType::value, bool> ChunkArrayBool;

protected:

	MAP& map_;
	ChunkArrayBool* mark_attribute_;

public:

	CellMarkerT(Map& map) :
		Inherit(),
		map_(map)
	{
		mark_attribute_ = map_.template get_mark_attribute<ORBIT>();
	}

	~CellMarkerT() override
	{
		if (MapGen::is_alive(&map_))
			map_.template release_mark_attribute<ORBIT>(mark_attribute_);
	}

	CellMarkerT(const Super& dm) = delete;
	CellMarkerT(Super&& dm) = delete;
	CellMarkerT<MAP, ORBIT>& operator=(Super&& dm) = delete;
	CellMarkerT<MAP, ORBIT>& operator=(const Super& dm) = delete;

	inline void mark(Cell<ORBIT> c)
	{
		cgogn_message_assert(mark_attribute_ != nullptr, "CellMarker has null mark attribute");
		mark_attribute_->set_true(map_.get_embedding(c));
	}

	inline void unmark(Cell<ORBIT> c)
	{
		cgogn_message_assert(mark_attribute_ != nullptr, "CellMarker has null mark attribute");
		mark_attribute_->set_false(map_.get_embedding(c));
	}

	inline void is_marked(Cell<ORBIT> c) const
	{
		cgogn_message_assert(mark_attribute_ != nullptr, "CellMarker has null mark attribute");
		return (*mark_attribute_)[map_.get_embedding(c)];
	}
};

template <typename MAP, unsigned int ORBIT>
class CellMarker : public CellMarkerT<MAP, ORBIT>
{
public:
	typedef CellMarkerT<MAP, ORBIT> Inherit;
	typedef CellMarker< MAP, ORBIT > Super;

	typedef typename Inherit::Orbit Orbit;
	typedef typename Inherit::Map Map;

	CellMarker(Map& map) :
		Inherit(map)
	{}

	~CellMarker() override
	{
		unmark_all() ;
	}

	CellMarker(const Super& dm) = delete;
	CellMarker(Super&& dm) = delete;
	CellMarker<MAP, ORBIT>& operator=(Super&& dm) = delete;
	CellMarker<MAP, ORBIT>& operator=(const Super& dm) = delete;

	inline void unmark_all()
	{
		cgogn_message_assert(this->mark_attribute_ != nullptr, "CellMarker has null mark attribute");
		this->mark_attribute_->all_false();
	}
};

template <typename MAP, unsigned int ORBIT>
class CellMarkerStore : public CellMarkerT<MAP, ORBIT>
{
public:
	typedef CellMarkerT<MAP, ORBIT> Inherit;
	typedef CellMarkerStore< MAP, ORBIT > Super;

	typedef typename Inherit::Orbit Orbit;
	typedef typename Inherit::Map Map;
	typedef typename Inherit::ProcessedCell ProcessedCell;
protected:

	std::vector<unsigned int>* marked_cells_;

public:

	CellMarkerStore(Map& map) :
		Inherit(map)
	{
		marked_cells_ = uint_buffers_thread->get_buffer();
	}

	~CellMarkerStore() override
	{
		unmark_all();
		uint_buffers_thread->release_buffer(marked_cells_);
	}

	CellMarkerStore(const Super& dm) = delete;
	CellMarkerStore(Super&& dm) = delete;
	CellMarkerStore<MAP, ORBIT>& operator=(Super&& dm) = delete;
	CellMarkerStore<MAP, ORBIT>& operator=(const Super& dm) = delete;

	inline void mark(Cell<ORBIT> c)
	{
		cgogn_message_assert(this->mark_attribute_ != nullptr, "CellMarker has null mark attribute");
		Inherit::mark(c);
		marked_cells_->push_back(this->map_.get_embedding(c));
	}

	inline void unmark_all()
	{
		cgogn_message_assert(this->mark_attribute_ != nullptr, "CellMarker has null mark attribute");
		for (unsigned int i : marked_cells_)
		{
			this->mark_attribute_->set_false(i);
		}
	}
};

} // namespace cgogn

#endif // CORE_BASIC_CELL_MARKER_H_
