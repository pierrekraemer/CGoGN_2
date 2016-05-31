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

#ifndef CGOGN_IO_NASTRAN_IO_H_
#define CGOGN_IO_NASTRAN_IO_H_

#include <map>
#include <sstream>
#include <iomanip>

#include <cgogn/core/utils/logger.h>
#include <cgogn/io/dll.h>
#include <cgogn/io/data_io.h>
#include <cgogn/io/volume_import.h>
#include <cgogn/io/volume_export.h>

namespace cgogn
{

namespace io
{

template <typename VEC3>
class NastranIO
{
public:

	using Scalar = typename geometry::vector_traits<VEC3>::Scalar;

	inline Scalar parse_scalar(std::string& str)
	{
		Scalar x(0);
		const std::size_t pos1 = str.find_last_of('-');
		if ((pos1!=std::string::npos) && (pos1!=0))
		{
			std::string res = str.substr(0,pos1) + "e" + str.substr(pos1,8-pos1);
			x = Scalar(std::stod(res));
		}
		else
		{
			const std::size_t pos2 = str.find_last_of('+');
			if ((pos2!=std::string::npos) && (pos2!=0))
			{
				std::string res = str.substr(0,pos2) + "e" + str.substr(pos2,8-pos2);
				x = Scalar(std::stod(res));
			}
			else
			{
				x = Scalar(std::stod(str));
			}
		}
		return x;
	}
};

template <typename MAP_TRAITS, typename VEC3>
class NastranVolumeImport : public NastranIO<VEC3>, public VolumeImport<MAP_TRAITS>
{
	using Inherit_Nastran = NastranIO<VEC3>;
	using Inherit_Import = VolumeImport<MAP_TRAITS>;
	using Self = NastranVolumeImport<MAP_TRAITS, VEC3>;
	template <typename T>
	using ChunkArray = typename Inherit_Import::template ChunkArray<T>;

	// MeshImportGen interface
protected:

	virtual bool import_file_impl(const std::string& filename) override
	{
		std::ifstream file(filename, std::ios::in);
		ChunkArray<VEC3>* position = this->template position_attribute<VEC3>();

		std::string line;
		line.reserve(512);
		std::string tag;
		tag.reserve(32);

		std::getline (file, line);
		do
		{
			std::getline (file, line);
			tag = line.substr(0,4);
		} while (tag !="GRID");

		// reading vertices
		std::map<uint32, uint32> old_new_ids_map;
		do
		{
			std::string s_v = line.substr(8,8);
			const uint32 old_index = std::stoi(s_v);
			const uint32 new_index = this->insert_line_vertex_container();
			old_new_ids_map[old_index] = new_index;
			auto& v = position->operator[](new_index);

			s_v = line.substr(24,8);
			v[0] = this->parse_scalar(s_v);
			s_v = line.substr(32,8);
			v[1] = this->parse_scalar(s_v);
			s_v = line.substr(40,8);
			v[2] = this->parse_scalar(s_v);

			std::getline (file, line);
			tag = line.substr(0,4);
			this->set_nb_vertices(this->nb_vertices() + 1u);
		} while (tag =="GRID");

		// reading volumes
		do
		{
			std::string s_v = line.substr(0, std::min(line.size(), std::size_t(12)));
			if (s_v[0] != '$')
			{
				if (s_v.compare(0, 5, "CHEXA") == 0)
				{
					this->set_nb_volumes(this->nb_volumes() + 1u);
					std::array<uint32, 8> ids;

					s_v = line.substr(24,8);
					ids[0] = uint32(std::stoi(s_v));
					s_v = line.substr(32,8);
					ids[1] = uint32(std::stoi(s_v));
					s_v = line.substr(40,8);
					ids[2] = uint32(std::stoi(s_v));
					s_v = line.substr(48,8);
					ids[3] = uint32(std::stoi(s_v));
					s_v = line.substr(56,8);
					ids[4] = uint32(std::stoi(s_v));
					s_v = line.substr(64,8);
					ids[5] = uint32(std::stoi(s_v));

					std::getline (file, line);
					s_v = line.substr(8,8);
					ids[6] = uint32(std::stoi(s_v));
					s_v = line.substr(16,8);
					ids[7] = uint32(std::stoi(s_v));

					for (uint32& id : ids)
						id = old_new_ids_map[id];

					this->add_hexa(*position, ids[0], ids[1], ids[2], ids[3], ids[4], ids[5],ids[6], ids[7], true);
				}
				else
				{
					if (s_v.compare(0, 6,"CTETRA") == 0)
					{
						this->set_nb_volumes(this->nb_volumes() + 1u);
						std::array<uint32, 4> ids;

						s_v = line.substr(24,8);
						ids[0] = uint32(std::stoi(s_v));
						s_v = line.substr(32,8);
						ids[1] = uint32(std::stoi(s_v));
						s_v = line.substr(40,8);
						ids[2] = uint32(std::stoi(s_v));
						s_v = line.substr(48,8);
						ids[3] = uint32(std::stoi(s_v));

						for (uint32& id : ids)
							id = old_new_ids_map[id];

						this->add_tetra(*position, ids[0], ids[1], ids[2], ids[3], true);
					}
					else
					{
						if (s_v.compare(0, 7,"ENDDATA") == 0)
							break;
						cgogn_log_warning("NastranVolumeImport") << "Elements of type \"" << s_v << "\" are not supported. Ignoring.";
					}
				}
			}

			std::getline (file, line);
			tag = line.substr(0,4);
		} while (!file.eof());

		return true;
	}
};

template <typename MAP>
class NastranVolumeExport : public VolumeExport<MAP>
{
public:

	using Inherit = VolumeExport<MAP>;
	using Self = NastranVolumeExport<MAP>;
	using Map = typename Inherit::Map;
	using Vertex = typename Inherit::Vertex;
	using Volume = typename Inherit::Volume;
	using ChunkArrayGen = typename Inherit::ChunkArrayGen;

protected:

	virtual void export_file_impl(const Map& map, std::ofstream& output, const ExportOptions& option) override
	{

		ChunkArrayGen const* pos = this->position_attribute();
		const std::string endianness = cgogn::internal::cgogn_is_little_endian ? "LittleEndian" : "BigEndian";
		const std::string format = (option.binary_?"binary" :"ascii");
		std::string scalar_type = pos->nested_type_name();
		scalar_type[0] = std::toupper(scalar_type[0]);

		output << "$$ ---------------------------------------------------------------------------- $"<< std::endl;
		output << "$$      NASTRAN MEsh File Generated by CGoGN_2 (ICube/IGG)                      $"<< std::endl;
		output << "$$ ---------------------------------------------------------------------------- $"<< std::endl;
		output << "CEND" << std::endl;
		output << "BEGIN BULK" << std::endl;
		output << "$$ ---------------------------------------------------------------------------- $"<< std::endl;
		output << "$$      Vertices position                                                       $"<< std::endl;
		output << "$$ ---------------------------------------------------------------------------- $"<< std::endl;

		// 1. vertices
		uint32 count{1u};
		map.foreach_cell([&] (Vertex v)
		{
			output << "GRID    ";
			output << std::right;
			output.width(8);
			output << count++;
			output << "        ";
			output << std::left;
			std::stringstream position_stream;
			pos->export_element(map.embedding(v), position_stream, false);
			float32 tmp[3];
			position_stream >> tmp[0];
			position_stream >> tmp[1];
			position_stream >> tmp[2];
			output << std::setw(8) << trunc_float_to8(tmp[0]) << std::setw(8) << trunc_float_to8(tmp[1]) << std::setw(8) << trunc_float_to8(tmp[2]) << std::endl;
		});

		count = 1u;
		auto vertices_it = this->vertices_of_volumes().begin();
		const auto& nb_vert_vol = this->number_of_vertices();
		const uint32 nb_vols = nb_vert_vol.size();
		output << std::right;

		if (this->nb_hexas() > 0u)
		{
			output << "$$ ---------------------------------------------------------------------------- $"<< std::endl;
			output << "$$      Hexa indices                                                            $"<< std::endl;
			output << "$$ ---------------------------------------------------------------------------- $"<< std::endl;

			for (uint32 w = 0u; w < nb_vols; ++w)
			{
				if (nb_vert_vol[w] == 8u)
				{
					output << "CHEXA   ";
					output << std::setw(8) << count++ << std::setw(8) << 0;
					output << std::setw(8) << (*vertices_it++ + 1u) << std::setw(8) << (*vertices_it++ + 1u) << std::setw(8) << (*vertices_it++ + 1u);
					output << std::setw(8) << (*vertices_it++ + 1u) << std::setw(8) << (*vertices_it++ + 1u) << std::setw(8) << (*vertices_it++ + 1u) << "+" << std::endl;
					output << "+       " << std::setw(8) << (*vertices_it++ + 1u) << std::setw(8) << (*vertices_it++ + 1u) << std::endl;
				}
				else
				{
					for (uint32 i = 0u; i < nb_vert_vol[w]; ++i)
						++vertices_it;
				}
			}
		}

		if (this->nb_tetras() > 0u)
		{
			vertices_it = this->vertices_of_volumes().begin();
			output << "$$ ---------------------------------------------------------------------------- $"<< std::endl;
			output << "$$      Tetra indices                                                           $"<< std::endl;
			output << "$$ ---------------------------------------------------------------------------- $"<< std::endl;

			for (uint32 w = 0u; w < nb_vols; ++w)
			{
				if (nb_vert_vol[w] == 4u)
				{
					output << "CTETRA  ";
					output << std::setw(8) << count++ << std::setw(8) << 0;
					output << std::setw(8) << (*vertices_it++ + 1u) << std::setw(8) << (*vertices_it++ + 1u) << std::setw(8) << (*vertices_it++ + 1u) << std::setw(8) << (*vertices_it++ + 1u) << std::endl;
				}
				else
				{
					for (uint32 i = 0u; i < nb_vert_vol[w]; ++i)
						++vertices_it;
				}
			}
		}
		output << "ENDDATA" << std::endl;
	}

private:

	static inline std::string trunc_float_to8(float32 f)
	{
		std::stringstream ss;
		ss << f;
		std::string res = ss.str();
		size_t expo = res.find('e');
		if (expo != std::string::npos)
		{
			if (res[expo+2] == '0')
				return res.substr(0, 6) + res[expo+1] + res[expo+3];

			return res.substr(0, 5) + res.substr(expo + 1);
		}
		return res.substr(0, 8);
	}
};

#if defined(CGOGN_USE_EXTERNAL_TEMPLATES) && (!defined(CGOGN_IO_NASTRAN_IO_CPP_))
extern template class CGOGN_IO_API NastranIO<Eigen::Vector3d>;
extern template class CGOGN_IO_API NastranIO<Eigen::Vector3f>;
extern template class CGOGN_IO_API NastranIO<geometry::Vec_T<std::array<float64,3>>>;
extern template class CGOGN_IO_API NastranIO<geometry::Vec_T<std::array<float32,3>>>;

extern template class CGOGN_IO_API NastranVolumeImport<DefaultMapTraits, Eigen::Vector3d>;
extern template class CGOGN_IO_API NastranVolumeImport<DefaultMapTraits, Eigen::Vector3f>;
extern template class CGOGN_IO_API NastranVolumeImport<DefaultMapTraits, geometry::Vec_T<std::array<float64,3>>>;
extern template class CGOGN_IO_API NastranVolumeImport<DefaultMapTraits, geometry::Vec_T<std::array<float32,3>>>;

extern template class CGOGN_IO_API NastranVolumeExport<CMap3<DefaultMapTraits>>;
#endif // defined(CGOGN_USE_EXTERNAL_TEMPLATES) && (!defined(CGOGN_IO_NASTRAN_IO_CPP_))

} // namespace io

} // namespace cgogn

#endif // CGOGN_IO_NASTRAN_IO_H_
