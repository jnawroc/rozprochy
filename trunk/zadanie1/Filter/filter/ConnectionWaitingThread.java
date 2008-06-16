package filter;

import java.io.IOException;
import java.io.InputStream;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

public class ConnectionWaitingThread extends Thread {
	private ServerSocket ssocket = null;

	private int port;

	private Map<Integer, Integer> timestsmpMap;

	public ConnectionWaitingThread(int port) {
		super();
		this.port = port;
	}

	@Override
	public void run() {
		try {
			ssocket = new ServerSocket(port);
			while (true) {
				Socket soc = ssocket.accept();
				timestsmpMap = new TreeMap<Integer, Integer>();
				InputStream is;
				is = soc.getInputStream();
				byte[] b = new byte[4];
				is.read(b);
				int port = (b[0]<<8) ;
				port+= (b[1]<0)?256+b[1]:b[1];
				b = new byte[4];
				is.read(b);
			
				InetAddress ia ;
				if (b[0]!=0){
					ia= InetAddress.getByAddress(b);
				}else{
					ia = soc.getInetAddress();
				}

				int type = 0;
				is.read(b);
				b = new byte[8];
				
				while (type != Commons.CONFIGURATION_END) {
					
					is.read(b);
					type = b[7];
					if (type == Commons.CONFIGURATION_END){
						break;
					}
					timestsmpMap.put(type, new Integer(b[0]));
				}
				new SendingThread(port,ia,timestsmpMap).start();
			}
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

}
