#include "../../SchemaRegistry.h"
#include "../../HDFFile.h"
#include "../../HDFFile_h5.h"
#include "schemas/amo0_psi_sinq_generated.h"

#include <iterator>
#include <iostream>

namespace BrightnESS {
namespace FileWriter {
namespace Schemas {
namespace AMOR {

template <typename T> hid_t nat_type();
template <> hid_t nat_type<uint32_t>() { return H5T_NATIVE_UINT32; }
template <> hid_t nat_type<uint64_t>() { return H5T_NATIVE_UINT64; }
template <> hid_t nat_type<int32_t>() { return H5T_NATIVE_INT32; }
template <> hid_t nat_type<int64_t>() { return H5T_NATIVE_INT64; }


class reader : public FBSchemaReader {
std::unique_ptr<FBSchemaWriter> create_writer_impl() override;
std::string sourcename_impl(Msg msg) override;
uint64_t ts_impl(Msg msg) override;
};


class writer : public FBSchemaWriter {
  const hsize_t chunk_size = 1000000;
~writer() override;
  void init_impl(std::string const &, hid_t, Msg) override;
  WriteResult write_impl(Msg) override;
  hid_t grp_event = -1;
  hid_t ds_event_index;
  hid_t ds_pulse_time;
  hid_t ds_time_of_flight;
  hid_t ds_detector_id;
  // hid_t dsp = -1; // dataspace
  // hid_t dpl = -1; // data properties list
  uint64_t pid = -1;
};

std::unique_ptr<FBSchemaWriter> reader::create_writer_impl() {
  return std::unique_ptr<FBSchemaWriter>(new writer);
}

std::string reader::sourcename_impl(Msg msg) {
  auto event = GetEventMessage(msg.data);
  
  if (!event->source_name()) {
    LOG(4, "WARNING message has no source name");
    return "";
  }
  return event->source_name()->str();
}

// TODO
// Should be in64_t to handle error cases
uint64_t reader::ts_impl(Msg msg) {
  auto event = GetEventMessage(msg.data);
  auto ts = event->pulse_time();
  if (!ts) {
    LOG(4, "ERROR no time data sent");
    return 0;
  }
  return ts;
}


writer::~writer() { }

  template<typename T>
  static hid_t create_dataset(hid_t loc, std::string name) {
    auto dt = nat_type<T>();
    std::array<hsize_t, 1> sizes_ini {{0}};
    std::array<hsize_t, 1> sizes_max {{H5S_UNLIMITED}};
    hid_t dsp = H5Screate_simple(sizes_ini.size(), sizes_ini.data(), sizes_max.data());
    
    if(true) {
      LOG(7, "\tDataSpace isSimple {}", H5Sis_simple(dsp));
      auto ndims = H5Sget_simple_extent_ndims(dsp);
      LOG(7, "\tDataSpace getSimpleExtentNdims {}", ndims);
      LOG(7, "\tDataSpace getSimpleExtentNpoints {}", 
	  H5Sget_simple_extent_npoints(dsp));
      std::vector<hsize_t> get_sizes_now;
      std::vector<hsize_t> get_sizes_max;
      get_sizes_now.resize(ndims);
      get_sizes_max.resize(ndims);
      H5Sget_simple_extent_dims(dsp, get_sizes_now.data(), get_sizes_max.data());
      for (int i1 = 0; i1 < ndims; ++i1) {
	LOG(7, "\tH5Sget_simple_extent_dims {:3} {:3}", 
	    get_sizes_now.at(i1), get_sizes_max.at(i1));
      }
    }
    std::array<hsize_t, 1> schk {{ std::max(4*1024*1024/H5Tget_size(dt), (size_t)1) }};
    hid_t dpl = H5Pcreate(H5P_DATASET_CREATE);
    H5Pset_chunk(dpl, schk.size(), schk.data());
    hid_t result = H5Dcreate1(loc,name.c_str(), dt, dsp, dpl);
    H5Sclose(dsp);
    H5Pclose(dpl);
    return result;
  }



void writer::init_impl(std::string const & sourcename, hid_t hdf_group, Msg msg) {
  LOG(6, "amo0::init_impl  v.size() == {}", chunk_size);
  
  this->ds_time_of_flight = create_dataset<uint32_t>(hdf_group, "event_time_offset");
  this->ds_detector_id    = create_dataset<uint32_t>(hdf_group, "event_id");
  this->ds_pulse_time     = create_dataset<uint64_t>(hdf_group, "event_time_zero");
  this->ds_event_index = create_dataset<uint64_t>(hdf_group, "event_index");

}


  std::array<hsize_t, 1> _get_size_now(const hid_t& ds) {
    using A = std::array<hsize_t, 1>;    

    auto tgt = H5Dget_space(ds);
    auto ndims = H5Sget_simple_extent_ndims(tgt);
    LOG(6, "DataSpace getSimpleExtentNdims {}", ndims);
    LOG(6, "DataSpace getSimpleExtentNpoints {}", 
	H5Sget_simple_extent_npoints(tgt));
    
    A get_sizes_now;
    A get_sizes_max;
    H5Sget_simple_extent_dims(tgt, get_sizes_now.data(), get_sizes_max.data());
    for (uint i1 = 0; i1 < get_sizes_now.size(); ++i1)
      LOG(6, "H5Sget_simple_extent_dims {:3} {:3}", get_sizes_now.at(i1), get_sizes_max.at(i1));
    H5Sclose(tgt);

    return get_sizes_now;
  }


  std::array<hsize_t, 1> _h5data_extend(const hid_t& ds, std::array<hsize_t, 1> new_sizes, const std::array<hsize_t, 1>& event_size) {

    for (uint i1 = 0; i1 < new_sizes.size(); ++i1)
      new_sizes.at(i1) += event_size.at(i1);
    if (H5Dextend(ds, new_sizes.data()) < 0) {
      LOG(0, "ERROR can not extend dataset");
      return {-1ul};
    }
    return std::move(new_sizes);
  }


  template<typename value_type>
  std::array<hsize_t, 1> _h5data_write(const hid_t& ds, std::array<hsize_t, 1> sizes_now, const std::array<hsize_t, 1>& event_size, 
				       const value_type* data, const uint offset=0) {
    hid_t tgt = H5Dget_space(ds);
    herr_t err = H5Sselect_hyperslab(tgt, H5S_SELECT_SET, sizes_now.data(), nullptr, 
				     event_size.data(), nullptr);
    if (err < 0) {
      LOG(0, "ERROR can not select mem hyperslab");
      return {-1ul};
    }
    auto mem = H5Screate_simple(event_size.size(), event_size.data(), nullptr);

    auto dt = nat_type<uint32_t>();
    err = H5Dwrite(ds, dt, mem, tgt, H5P_DEFAULT, data);
    if (err < 0) {
      LOG(0, "ERROR writing failed");
      return {-1ul};
    }
    H5Sclose(mem);
    H5Sclose(tgt);
    return _get_size_now(ds);
    
  }



  template<typename T>
  hsize_t do_write(hid_t ds, T const * data, size_t nlen) {
    using A = std::array<hsize_t, 1>;
    A get_sizes_now = _get_size_now(ds);
    A new_sizes = _h5data_extend(ds, get_sizes_now, {{nlen}} );
    get_sizes_now = _h5data_write(ds,get_sizes_now, {{nlen}},data);
    if( new_sizes != get_sizes_now )
      LOG(6,"Expected file size differs from actual size");
    return get_sizes_now[0];
  }





WriteResult writer::write_impl(Msg msg) {
  auto event = GetEventMessage(msg.data);
  if( (event->message_id() != pid+1) && (pid < -1ul) ) {
    LOG(7, "amo0 stream event loss: {} -> {}", pid, event->message_id());
    // TODO write into nexus log
  }
  pid = event->message_id();
  uint32_t size=event->detector_id()->size();
  int64_t value = event->pulse_time();
  hsize_t position = do_write(this->ds_time_of_flight,event->time_of_flight()->data(),size);
  do_write(this->ds_detector_id, event->detector_id()->data(), size);
  do_write(this->ds_pulse_time , &value                      , 1);
  do_write(this->ds_event_index, &position                   , 1);
  return {value};
}



class Info : public SchemaInfo {
public:
FBSchemaReader::ptr create_reader() override;
};

FBSchemaReader::ptr Info::create_reader() {
  return FBSchemaReader::ptr(new reader);
}


SchemaRegistry::Registrar<Info> g_registrar(fbid_from_str("amo0"));


}
}
}
}