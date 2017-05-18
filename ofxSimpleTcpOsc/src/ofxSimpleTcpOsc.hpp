#pragma once

#include "ofxNetwork.h"
#include "ofxOsc.h"

#define MAXSENDSIZE 327680
#define MAXRECIVESIZE 100

namespace ofxSimpleTcpOsc {

	typedef  osc::OutboundPacketStream OSCSenderPacket;

	static void serializeOscPacket(OSCSenderPacket &packet, ofxOscMessage &message) {
		packet << osc::BeginMessage(message.getAddress().c_str());
		for (int i = 0; i < message.getNumArgs(); ++i)
		{
			if (message.getArgType(i) == OFXOSC_TYPE_INT32)
				packet << message.getArgAsInt32(i);
			else if (message.getArgType(i) == OFXOSC_TYPE_INT64)
				packet << (osc::int64)message.getArgAsInt64(i);
			else if (message.getArgType(i) == OFXOSC_TYPE_FLOAT)
				packet << message.getArgAsFloat(i);
			else if (message.getArgType(i) == OFXOSC_TYPE_DOUBLE)
				packet << message.getArgAsDouble(i);
			else if (message.getArgType(i) == OFXOSC_TYPE_STRING || message.getArgType(i) == OFXOSC_TYPE_SYMBOL)
				packet << message.getArgAsString(i).c_str();
			else if (message.getArgType(i) == OFXOSC_TYPE_CHAR)
				packet << message.getArgAsChar(i);
			else if (message.getArgType(i) == OFXOSC_TYPE_MIDI_MESSAGE)
				packet << message.getArgAsMidiMessage(i);
			else if (message.getArgType(i) == OFXOSC_TYPE_TRUE || message.getArgType(i) == OFXOSC_TYPE_FALSE)
				packet << message.getArgAsBool(i);
			else if (message.getArgType(i) == OFXOSC_TYPE_TRIGGER)
				packet << message.getArgAsTrigger(i);
			else if (message.getArgType(i) == OFXOSC_TYPE_TIMETAG)
				packet << (osc::int64)message.getArgAsTimetag(i);
			//else if ( message.getArgType( i ) == OFXOSC_TYPE_RGBA_COLOR )
			//	p << message.getArgAsRgbaColor( i );
			else if (message.getArgType(i) == OFXOSC_TYPE_BLOB) {
				ofBuffer buff = message.getArgAsBlob(i);
				osc::Blob b(buff.getData(), (unsigned long)buff.size());
				packet << b;
			}
			else
			{
				ofLogError("ofxOscSender") << "appendMessage(): bad argument type " << message.getArgType(i);
				assert(false);
			}
		}
		packet << osc::EndMessage;
	}

	static void deserializeOscPacket(const char *packet, int size, ofxOscMessage &msg) {

		//Init ofxOscMessage
		msg = ofxOscMessage();

		//Analysis raw packet
		osc::ReceivedPacket rp(packet, size);
		osc::ReceivedMessage receivedMsg(rp);

		//Osc Adress
		msg.setAddress(receivedMsg.AddressPattern());

		// transfer the arguments
		for (osc::ReceivedMessage::const_iterator arg = receivedMsg.ArgumentsBegin();
			arg != receivedMsg.ArgumentsEnd();
			++arg)
		{
			if (arg->IsInt32())
				msg.addIntArg(arg->AsInt32Unchecked());
			else if (arg->IsInt64())
				msg.addInt64Arg(arg->AsInt64Unchecked());
			else if (arg->IsFloat())
				msg.addFloatArg(arg->AsFloatUnchecked());
			else if (arg->IsDouble())
				msg.addDoubleArg(arg->AsDoubleUnchecked());
			else if (arg->IsString())
				msg.addStringArg(arg->AsStringUnchecked());
			else if (arg->IsSymbol())
				msg.addSymbolArg(arg->AsSymbolUnchecked());
			else if (arg->IsChar())
				msg.addCharArg(arg->AsCharUnchecked());
			else if (arg->IsMidiMessage())
				msg.addMidiMessageArg(arg->AsMidiMessageUnchecked());
			else if (arg->IsBool())
				msg.addBoolArg(arg->AsBoolUnchecked());
			else if (arg->IsInfinitum())
				msg.addTriggerArg();
			else if (arg->IsTimeTag())
				msg.addTimetagArg(arg->AsTimeTagUnchecked());
			else if (arg->IsRgbaColor())
				msg.addRgbaColorArg(arg->AsRgbaColorUnchecked());
			else if (arg->IsBlob()) {
				const char * dataPtr;
				osc::osc_bundle_element_size_t len = 0;
				arg->AsBlobUnchecked((const void*&)dataPtr, len);
				ofBuffer buffer(dataPtr, len);
				msg.addBlobArg(buffer);
			}
			else
			{
				ofLogError("ofxOscReceiver") << "ProcessMessage: argument in message " << receivedMsg.AddressPattern() << " is not an int, float, or string";
			}
		}
	}

	class server {
	public:
		ofEvent<ofxOscMessage> updateEvent;

		void setup(int port) {
			tcpServer.close();
			tcpServer.setup(port, false);
			tcpServer.setMessageDelimiter("\n");

			ofLog() << "Setup server";
		}

		void update() {
			if (tcpServer.isConnected() && tcpServer.getNumClients() > 0) {
				char buffer[MAXRECIVESIZE];
				int size = tcpServer.receiveRawBytes(0, buffer, MAXRECIVESIZE);
				if (size > 0) {
					ofxOscMessage msg;
					deserializeOscPacket(buffer, size, msg);
					ofNotifyEvent(updateEvent, msg);
				}
			}
		}

		void send(ofxOscMessage &message, int ClientID) {
			if (tcpServer.isConnected() && tcpServer.getNumClients() > 0) {
				char buffer[MAXSENDSIZE];
				OSCSenderPacket packet(buffer, MAXSENDSIZE);
				serializeOscPacket(packet, message);
				if (tcpServer.sendRawBytes(ClientID, packet.Data(), packet.Size()) == false) {
					ofLogError() << "[Server] Send error";
				}
				else {
					ofLogVerbose() << "[Server] Send succsess";
				}
			}
		}

		void close() {
			tcpServer.close();
		}

	private:
		ofxTCPServer tcpServer;
	};

	class client {
	public:
		ofEvent<ofxOscMessage> updateEvent;

		void setup(string host, int port) {
			tcpClient.close();
			tcpClient.setup(host, port, false);
			tcpClient.setMessageDelimiter("\n");

			ofLog() << "Setup client";
		}

		void update() {
			if (tcpClient.isConnected()) {
				char buffer[MAXRECIVESIZE];
				int size = tcpClient.receiveRawBytes(buffer, 100);
				if (size > 0) {
					ofxOscMessage msg;
					deserializeOscPacket(buffer, size, msg);
					ofNotifyEvent(updateEvent, msg);
				}
			}
		}

		void send(ofxOscMessage &message) {
			if (tcpClient.isConnected()) {
				char buffer[MAXSENDSIZE];
				OSCSenderPacket packet(buffer, MAXSENDSIZE);
				serializeOscPacket(packet, message);
				if (tcpClient.sendRawBytes(packet.Data(), packet.Size()) == false) {
					ofLogError() << "[Client] Send error";
				}
			}
			else {
				ofLogVerbose() << "[Client] Send succsess";
			}
		}

		void close() {
			tcpClient.close();
		}

	private:
		ofxTCPClient tcpClient;
	};
}
