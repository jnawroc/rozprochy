package filter;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.SocketException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Random;

public class SendingThread extends Thread {
	private int port;

	private InetAddress ia;

	private Map<Integer, Integer> timestsmpMap;

	private int i;

	private Random random = new Random();

	private DatagramSocket ds;

	public SendingThread(int port, InetAddress ia,
			Map<Integer, Integer> timestsmpMap) {
		super();
		this.port = port;
		this.ia = ia;
		this.timestsmpMap = timestsmpMap;
		try {
			ds = new DatagramSocket();
			ds.connect(ia, port);
		} catch (SocketException e) {
			e.printStackTrace();
		}
		i = 0;
	}

	@Override
	public void run() {
		try {
			while (true) {
				sleep(1000);
				for (int j : timestsmpMap.keySet()) {
					if (i % (timestsmpMap.get(j)) == 0) {
						List<byte[]> l = new ArrayList<byte[]>();
						for (InetSocketAddress ia :Filter.listMap.get(j).keySet()){
							l.add( Filter.listMap.get(j).get(ia));
						}
						if (l.size() > 0) {
							byte ba[] = l.get(random.nextInt(l.size()));
							DatagramPacket dp = new DatagramPacket(ba,
									ba.length, ia, port);
							ds.send(dp);
							System.out.println("Wysyłam");
						}
					}
				}
				i++;
			}
		} catch (InterruptedException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

}
