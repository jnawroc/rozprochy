package filter;

import java.io.OutputStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

import com.martiansoftware.jsap.FlaggedOption;
import com.martiansoftware.jsap.JSAP;
import com.martiansoftware.jsap.JSAPResult;
import com.martiansoftware.jsap.Parameter;
import com.martiansoftware.jsap.SimpleJSAP;
import com.martiansoftware.jsap.UnflaggedOption;

/**
 * Filtr - to program, który zbiera informacje z zadanych Emiterów i produkuje
 * nową treść np.: max. liczba uruchomionych procesów, maksymalna ilość wolnej
 * pamięci. Filtr posiada dokładnie takie same kanały komunikacyjne jak Emiter,
 * co powinno pozwalać łączyć Filtry w kaskady, a także podłączać do Filtra
 * program Obserwatora. Program Filtra jako argumenty powinien przyjmować 1 lub
 * więcej docelowych adresów transportowych węzła Emitera lub Filtra. Program
 * Filtra powinien obsługiwać opcjonalny argument wywołania '-m ', który podany
 * powoduje nasłuch na dane w trybie multicast. Przykładowa linia poleceń może
 * wyglądać następująco: filtr 1.2.3.4/5000 2.3.4.5/3000 2.2.2.2/6500 co powinno
 * spowodować, że Filtr zgłosi się do trzech programów (Emiterów i/lub Filtrów)
 * i będzie agregował zbierane od nich informacje zgodnie z wybranymi kryteriami
 * np. maksymalna liczba procesów, minimalna liczba procesów itp., filtr -m
 * 239.0.3.3/5000 1.2.3.4/5000 2.3.4.5/2222 powinno spowodować, że Filtr włączy
 * nasłuch na adresie 239.0.3.3/5000 i zgłosi się do dwóch programów agregując
 * odbierane dane. Filtr w tym przypadku musi zażądać od obu programów nadawania
 * w trybie multicast na wskazany adres.
 * 
 * @author kuba
 * 
 */
public class Filter {

	/**
	 * @param args
	 */
	private static List<Socket> emiters = new ArrayList<Socket>();

	private static List<InetSocketAddress> IPList = new ArrayList<InetSocketAddress>();

	private static boolean hasMulticast;

	private static InetAddress mip;

	private static int mp;

	private static int myPort;

	public static Map<Integer, Map<InetSocketAddress,byte[]>> listMap = new TreeMap<Integer, Map<InetSocketAddress,byte[]>>();

	private static int port = 8787;

	public static void main(String[] args) {
		try {
			Parameter[] params;
			params = new Parameter[] {
					new FlaggedOption("multicast", JSAP.STRING_PARSER,
							JSAP.NO_DEFAULT, JSAP.NOT_REQUIRED, 'm',
							"multicast", "Adres multicastowy."),
					new FlaggedOption("all", JSAP.INTEGER_PARSER,
							JSAP.NO_DEFAULT, JSAP.NOT_REQUIRED, 'a', "all",
							"zbiera wszystkie statystyki")
							.setUsageName("timestamp"),
					new FlaggedOption("cpu", JSAP.INTEGER_PARSER,
							JSAP.NO_DEFAULT, JSAP.NOT_REQUIRED, 'c', "cpu",
							"nazwa(y) procesora(ów)").setUsageName("timestamp"),
					new FlaggedOption("memory", JSAP.INTEGER_PARSER,
							JSAP.NO_DEFAULT, JSAP.NOT_REQUIRED, 'M', "memory",
							"statystyki pamięci").setUsageName("timestamp"),
					new FlaggedOption("proc", JSAP.INTEGER_PARSER,
							JSAP.NO_DEFAULT, JSAP.NOT_REQUIRED, 'p', "proc",
							"liczba procesów").setUsageName("timestamp"),
					new FlaggedOption("load", JSAP.INTEGER_PARSER,
							JSAP.NO_DEFAULT, JSAP.NOT_REQUIRED, 'l', "load",
							"obciążenie systemu").setUsageName("timestamp"),
					new FlaggedOption("port", JSAP.INTEGER_PARSER,
							JSAP.NO_DEFAULT, JSAP.REQUIRED, 'P', "port",
							"port na którym nasłuchuje").setUsageName("port"),
					new UnflaggedOption("address", JSAP.STRING_PARSER,
							JSAP.NO_DEFAULT, JSAP.REQUIRED, JSAP.GREEDY,
							"Adresy IP emiterów/filtrów z którymi się ma łączyć") };

			String opis = "program, który zbiera informacje z zadanych Emiterów i produkuje nową treść";
			SimpleJSAP jsap = new SimpleJSAP("Filter", opis, params);
			JSAPResult config = jsap.parse(args);
			if (jsap.messagePrinted())
				System.exit(1);
			String[] IPs = config.getStringArray("address");
			hasMulticast = config.contains("multicast");
			if (hasMulticast) {
				String[] str = config.getString("multicast").split("/");
				mip = InetAddress.getByName(str[0]);
				mp = Integer.valueOf(str[1]);
			}
			myPort = config.getInt("port");
			for (String s : IPs) {
				String[] arr = s.split("/");
				InetAddress ia = InetAddress.getByName(arr[0]);
				Integer port = Integer.valueOf(arr[1]);
				emiters.add(new Socket(ia, port));
				InetSocketAddress sa = new InetSocketAddress(ia, port);
				IPList.add(sa);
			}
			Thread.sleep(400);
			for (Socket soc : emiters) {
				OutputStream os = (soc.getOutputStream());
				if (hasMulticast) {
					System.out.println("JEST MULTICAST");
					int hi = mp >> 8;
					int lo = mp & ((1 << 8) - 1);
					os.write(hi);
					os.write(lo);
					os.write(0);
					os.write(0);
					os.write(mip.getAddress());
					os.flush();
				} else {
					byte[] arr= new byte[8];
					arr[0]=arr[1]=16;
					arr[2]=arr[3]=arr[4]=arr[5]=arr[6]=arr[7]=0;
					//= { 16, 16, 0, 0, 0, 0 ,0,0};
					os.write(arr, 0, arr.length);
					os.flush();

				}
				byte[] arr_ = new byte[4];
				os.write(arr_, 0, arr_.length);
				os.flush();
				if (config.contains("all")) {
					Integer timestamp = config.getInt("all");

					byte[] arr = new byte[8];
					arr[0] = arr[1] = arr[2] = arr[3] = arr[4] = arr[5] = arr[6] = arr[7] = 0;
					arr[7] = Commons.LOAD;
					arr[0] = timestamp.byteValue();
					os.write(arr, 0, arr.length);
					os.flush();
					arr = new byte[8];
					arr[0] = arr[1] = arr[2] = arr[3] = arr[4] = arr[5] = arr[6] = arr[7] = 0;
					arr[0] = timestamp.byteValue();
					arr[7] = Commons.MEMORY;
					os.write(arr, 0, arr.length);
					os.flush();
					arr = new byte[8];
					arr[0] = arr[1] = arr[2] = arr[3] = arr[4] = arr[5] = arr[6] = arr[7] = 0;
					arr[0] = timestamp.byteValue();
					arr[7] = Commons.PROC;
					os.write(arr, 0, arr.length);
					os.flush();
					arr = new byte[8];
					arr[0] = arr[1] = arr[2] = arr[3] = arr[4] = arr[5] = arr[6] = arr[7] = 0;
					arr[0] = timestamp.byteValue();
					arr[7] = Commons.CPU_NAME;

					os.write(arr, 0, arr.length);
					os.flush();
;
				} else {
					if (config.contains("proc")) {
						Integer timestamp = config.getInt("proc");
						byte[] arr = {timestamp.byteValue(), 0, 0, 0,
								0,0,0, Commons.PROC};
						os.write(arr, 0, arr.length);
						os.flush();
					}
					if (config.contains("memory")) {
						Integer timestamp = config.getInt("memory");
						byte[] arr = {timestamp.byteValue(), 0, 0, 0,
								0,0,0, Commons.MEMORY};
						os.write(arr, 0, arr.length);
						os.flush();
					}
					if (config.contains("load")) {
						Integer timestamp = config.getInt("load");
						byte[] arr = {timestamp.byteValue(), 0, 0, 0,
								0,0,0, Commons.LOAD};
						os.write(arr, 0, arr.length);
						os.flush();
					}
					if (config.contains("cpu")) {
						Integer timestamp = config.getInt("cpu");
						byte[] arr = {timestamp.byteValue(), 0, 0, 0,
								0,0,0, Commons.CPU_NAME};
						os.write(arr, 0, arr.length);
						os.flush();
					}
				}
				byte[] arr = new byte[8];
				arr[0] = arr[1] = arr[2] = arr[3] = arr[4] = arr[5] = arr[6] = arr[7] = 0;
				arr[7] = Commons.CONFIGURATION_END;
//				byte[] arr = { 0, 0, 0, Commons.CONFIGURATION_END, 0, 0, 0, 0 };
				os.write(arr, 0, arr.length);
				os.flush();
				os.close();
				new ConnectionWaitingThread(myPort).start();
				DatagramSocket UDPsoc = new DatagramSocket(hasMulticast?mp:4112);
				UDPsoc.setReuseAddress(true);
//				UDPsoc.setBroadcast(true);
				for (int i = 0; i < 5; i++) {
					listMap.put(i, new HashMap<InetSocketAddress,byte[]>());
				}
				while (true) {



					for (InetSocketAddress ia : IPList) {
						try {
							UDPsoc.setSoTimeout(1000);
							//UDPsoc.connect(ia);
							byte[] barr = new byte[4 + Commons.UDP_MSG_LEN];

							DatagramPacket dp = new DatagramPacket(barr,
									barr.length);
							UDPsoc.receive(dp);
							barr = dp.getData();
							int type = 0//(((((barr[0] << 8) + barr[1]) << 8) + barr[2]) << 8)
									+ barr[0];
//							for (int i = 0 ;i<barr.length;i++){
//								System.out.println(String.valueOf(i)+"["+barr[i]+"]");
//							}
							listMap.get(type).put(ia,barr);
						} catch (SocketTimeoutException ex) {
							// ex.printStackTrace();
						}
					}
					Thread.sleep(1000);
				}
			}
		} catch (Exception e) {
			e.printStackTrace(System.err);
		}
	}

}
