#include <iostream>
// #include <cstdlib>
#include <stdexcept>
#include <arpa/inet.h> // aton()

#include "Net_sniffer.h"
#include "Signature_analysis.h"

using namespace std;

Net_sniffer::Net_sniffer() : is_live(true), filter_exp("ip") {
}

Net_sniffer::Net_sniffer(const string& device, const string& protocol, bool mode) :
	device(device), filter_exp(protocol), is_live(mode) {
}

void Net_sniffer::start_sniff() {
		if (device.empty()) {
				char * dev = pcap_lookupdev(errbuf);
				if (dev == nullptr) {
						string reason = "Couldn't find default device ";
						throw runtime_error(reason + errbuf);
				}
				device = dev;
		}

		if (is_live) {
				if (pcap_lookupnet(device.c_str(), &net, &mask, errbuf) == -1) {
						string reason = "Couldn't get netmask for device ";
						throw runtime_error(reason + errbuf);
				}
				handle = pcap_open_live(device.c_str(), SNAP_LEN, 1, 1000, errbuf);
		}	else {
				handle = pcap_open_offline(device.c_str(), errbuf);
		}

		if (handle == nullptr) {
				string reason = "Couldn't open device ";
				throw runtime_error(reason + errbuf);
		}

		if (pcap_datalink(handle) != DLT_EN10MB) { // some information about linktype http://www.manpagez.com/man/7/pcap-linktype/
				string reason = "It's not an Ethernet ";
				throw runtime_error(reason);
		}

		if (pcap_compile(handle, &fp, filter_exp.c_str(), 1, net) == -1) {
				throw runtime_error("Couldn't parse filter");
		}

		if (pcap_setfilter(handle, &fp) == -1) {
				throw runtime_error("Couldn't install filter");
		}

		if (is_live) {
				cout << "Device " << device << endl;
				cout << "Filter " << filter_exp.c_str() << endl;
		}

		Signature_analysis sa("xml/configurations.xml", "debc", "1.pcap");

		pcap_loop(handle, 0, got_packet, (u_char *)&sa);
		pcap_freecode(&fp);
		pcap_close(handle);
		sa.print_sessions_list();
}

void Net_sniffer::got_packet(u_char * args, const struct pcap_pkthdr * header, const u_char * packet) {
		//Working_classes *wc = (Working_classes *) args;

		Packet pack(header);
		pack.parse(packet);

		static size_t num = 0;
		if (!pack.is_broken) {
				// cout << "Packet number: " << ++num << endl;
				// cout << "Source ip: " << inet_ntoa(pack.get_ip().ip_src) << endl;
				// cout << "Destenation ip: " << inet_ntoa(pack.get_ip().ip_dst) << endl;
				// cout << pack.get_pload() << endl << endl << endl;
				Signature_analysis *sa = (Signature_analysis *)args;
				sa->add_packet(pack);
		}
}
