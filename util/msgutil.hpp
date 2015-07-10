#ifndef MICROPTP_UTIL_MSGUTIL_HPP__
#define MICROPTP_UTIL_MSGUTIL_HPP__

#include <cstring>
#include <microptp/util/utiltypes.hpp>
#include <microptp/types.hpp>
#include <microptp/ports/systemport_defaults.hpp>

namespace uptp {

	template< bool Signed >
	struct nibble_upper_type {};

	template< bool Signed >
	struct nibble_lower_type {};

	using uint4u = nibble_upper_type<false>;
	using uint4l = nibble_lower_type<false>;

	using int4u = nibble_upper_type<true>;
	using int4l = nibble_lower_type<true>;

	
	namespace detail {

		template< typename T >
		T& reinterpret(void* ptr, size_t offset)
		{
			return *reinterpret_cast<T*>(reinterpret_cast<char*>(ptr) + offset);
		}

		template< typename T >
		const T& reinterpret(const void* ptr, size_t offset)
		{
			return *reinterpret_cast<const T*>(reinterpret_cast<const char*>(ptr) + offset);
		}

		inline void* offset(void* buff, size_t offset)
		{
			return reinterpret_cast<uint8*>(buff) + offset;
		}

		inline const void* offset(const void* buff, size_t offset)
		{
			return reinterpret_cast<const uint8*>(buff) + offset;
		}
		
		template< typename PacketType, typename HostType, size_t Offset, size_t Alignment >
		struct xchange {

			static void serialize(void* buffer, const HostType& host) {
				if (Alignment % std::alignment_of<PacketType>::value == 0) {
					reinterpret<PacketType>(buffer, Offset) = static_cast<PacketType>(system::to_net_byteorder(host));
				} else {
					PacketType temp = static_cast<PacketType>(system::to_net_byteorder(host));
					memcpy(offset(buffer, Offset), &temp, sizeof(PacketType));
				}
			}

			static void deserialize(const void* buffer, HostType& host) {
				if (Alignment % std::alignment_of<PacketType>::value == 0) {
					host = static_cast<HostType>(system::to_host_byteorder(reinterpret<PacketType>(buffer, Offset)));
				} else {
					PacketType temp;
					memcpy(&temp, offset(buffer, Offset), sizeof(PacketType));
					host = static_cast<HostType>(system::to_host_byteorder(temp));
				}
			}

		};

		template<typename HostType, size_t Offset, size_t Alignment>
		struct xchange< uint4u, HostType, Offset, Alignment > {
			static void serialize(void* buffer, const HostType& host) {
				reinterpret<uint8>(buffer, Offset) = (reinterpret<uint8>(buffer, Offset) & 0x0F) | ((host & 0x0F) << 4);
			}

			static void deserialize(const void* buffer, HostType& host) {
				host = (reinterpret<uint8>(buffer, Offset) & 0xF0) >> 4;
			}
		};

		template< typename HostType, size_t Offset, size_t Alignment >
		struct xchange< uint4l, HostType, Offset, Alignment > {
			static void serialize(void* buffer, const HostType& host) {
				reinterpret<uint8>(buffer, Offset) = (reinterpret<uint8>(buffer, Offset) & 0xF0) | (host & 0x0F);
			}

			static void deserialize(const void* buffer, HostType& host) {
				host = reinterpret<uint8>(buffer, Offset) & 0x0F;
			}
		};

		template< typename HostType, size_t Offset, size_t Alignment >
		struct xchange< int4u, HostType, Offset, Alignment > {
			static void serialize(void* buffer, const HostType& host) {
				reinterpret<uint8>(buffer, Offset) = (reinterpret<uint8>(buffer, Offset) & 0x0F) | ((host & 0x0F) << 4);
			}

			static void deserialize(const void* buffer, HostType& host) {
				host = (reinterpret<uint8>(buffer, Offset) & 0xF0) >> 4;
				host = (host & 8) ? (host | 0xF0) : host;
			}
		};

		template< typename HostType, size_t Offset, size_t Alignment >
		struct xchange< int4l, HostType, Offset, Alignment > {
			static void serialize(void* buffer, const HostType& host) {
				reinterpret<uint8>(buffer, Offset) = (reinterpret<uint8>(buffer, Offset) & 0xF0) | (host & 0x0F);
			}

			static void deserialize(const void* buffer, HostType& host) {
				host = (reinterpret<uint8>(buffer, Offset) & 0x0F);
				host = (host & 8) ? (host | 0xF0) : host;
			}
		};

		// Arrays
		template< typename HostType, size_t Offset, typename TargetType, size_t Size, size_t Alignment >
		struct xchange< TargetType[Size], HostType, Offset, Alignment >
		{
			template< size_t Index >
			static void write(void* buffer, const HostType& host, util::size_t_type<Index>)
			{
				xchange<TargetType, std::decay_t<decltype(host[0])>, Offset + Index * sizeof(TargetType), Alignment>::serialize(buffer, host[Index]);
				write(buffer, host, util::size_t_type<Index + 1>());
			}

			static void write(void* buffer, const HostType& host, util::size_t_type<Size>)
			{
			}

			static void serialize(void* buffer, const HostType& host)
			{
				write(buffer, host, util::size_t_type<0>());
			}

			template< size_t Index >
			static void read(const void* buffer, HostType& host, util::size_t_type<Index>)
			{
				xchange<TargetType, std::decay_t<decltype(host[0])>, Offset + Index * sizeof(TargetType), Alignment>::deserialize(buffer, host[Index]);
				read(buffer, host, util::size_t_type<Index + 1>());
			}


			static void read(const void* buffer, HostType& host, util::size_t_type<Size>)
			{
			}

			static void deserialize(const void* buffer, HostType& host)
			{
				read(buffer, host, util::size_t_type<0>());
			}
		};

	}
	
	struct net_const_buffer {
		net_const_buffer(const void* ptr)
			: ptr_(ptr)
		{
		}

		net_const_buffer offset(size_t off) {
			return net_const_buffer(detail::offset(ptr_, off));
		}

		template< typename PacketType, size_t offset, typename HostType >
		net_const_buffer& deserialize(HostType& host)
		{
			detail::xchange<PacketType, HostType, offset, 2>::deserialize(ptr_, host);
			return *this;
		}

		template< typename NestedType, void (*Deserialization)(net_const_buffer, NestedType&) >
		net_const_buffer& deserialize_nested(NestedType& host)
		{
			Deserialization(*this, host);
			return *this;
		}

	private:
		const void* ptr_;
	};

	struct net_buffer {
		net_buffer(void* ptr)
			: ptr_(ptr)
		{
		}

		net_buffer offset(size_t off) {
			return net_buffer(detail::offset(ptr_, off));
		}

		template< typename PacketType, size_t offset, typename HostType >
		net_buffer& deserialize(HostType& host)
		{
			detail::xchange<PacketType, HostType, offset, 2>::deserialize(ptr_, host);
			return *this;
		}

		template< typename PacketType, size_t offset, typename HostType >
		net_buffer& serialize(const HostType& host)
		{
			detail::xchange<PacketType, HostType, offset, 2>::serialize(ptr_, host);
			return *this;
		}

		template< typename NestedType, void (*Serialization)(net_buffer, const NestedType&) >
		net_buffer& serialize_nested(const NestedType& host)
		{
			Serialization(*this, host);
			return *this;
		}

	private:
		void* ptr_;
	};

}

#endif
