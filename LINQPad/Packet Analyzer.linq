<Query Kind="Program">
  <Namespace>System.Globalization</Namespace>
  <Namespace>System.IO.Pipes</Namespace>
</Query>

const string PipeName = "XyErebus2.Bridge";

void Main()
{
	//FIXME : game hang when current is stopped;
	StartService();
}

void StartService()
{	
	var service = new NamedPipeServerStream(PipeName, PipeDirection.In, 253);
    var reader = new StreamReader(service);
	
	do
	{
		try
		{
			Console.WriteLine(">> Waiting for connection...");
			service.WaitForConnection();
			Console.WriteLine(">> Connection established.");
			
			while (true)
			{
				ProcessMessage(reader.ReadLine().Dump());
			}
		}
		catch(Exception e)
		{
			e.Dump("An error has occured");
		}
		finally
		{
			service.WaitForPipeDrain();
			if (service.IsConnected)
			{
				service.Disconnect();
			}
			
			Console.WriteLine(">> Connection closed.");
		}
	}
	while(true);
}

void ProcessMessage(string rawMessage)
{
	try
	{
		var message = new Message(rawMessage);
		switch((int)message.Type)
		{
			case 64: //Move to Location
			case 68: //Move a Step
				return;
				
			case 81:  //not sure what this is, but it get spammed a lot
			case 109: //same
				return;
		
			case 204: //notify charging spell
			case 61: //appears related to spell finish charging (sometime gets sent twice
				break;
		
			case 219: //@69D0E6 cast spell (targeted location teleport)
			case 203: //@696584 cast spell (linear aoe)
			case 209: //@697087 cast spell (chain lightning)
					//    * chain lightning sends a 209 per target hits
				break;
		}
		
		message.Dump();
	}
	catch(Exception e)
	{
		e.Dump();
	}
}

public enum PacketType
{
}

public class Message : ICustomMemberProvider
{
	public int Caller { get; private set; }
	public PacketType Type { get; private set; }
	public int Size { get; private set; }
	public byte[] Data { get; private set; }

	public Message(string rawMessage)
	{
		var match = Regex.Match(rawMessage, @"packet{caller = (?<caller>[\da-f]{8}), type = (?<type>[\da-f]{4}), size = (?<size>[\da-f]{4}), data = (?<data>[\da-f]+)};", RegexOptions.IgnoreCase);
		
		Caller = int.Parse(match.Groups["caller"].Value, NumberStyles.HexNumber);
		Type = (PacketType)int.Parse(match.Groups["type"].Value, NumberStyles.HexNumber);
		Size = int.Parse(match.Groups["size"].Value, NumberStyles.HexNumber);
		Data = Enumerable.Range(0, Size)
			.Select(i => byte.Parse(match.Groups["data"].Value.Substring(i * 2, 2), NumberStyles.HexNumber)).ToArray();
	}
	
	#region ICustomMemberProvider members
	
    IEnumerable<string> ICustomMemberProvider.GetNames() 
    {
		return this.GetType().GetProperties()
			.Select(p => p.Name);
    }

    IEnumerable<Type> ICustomMemberProvider.GetTypes ()
    {
		return this.GetType().GetProperties()
			.Select(p => p.PropertyType);
    }

    IEnumerable<object> ICustomMemberProvider.GetValues ()
    {
		var result = this.GetType().GetProperties()
			.ToDictionary(p => p.Name, p => p.GetValue(this));
		result["Caller"] = Caller.ToString("X8");
		result["Type"] = string.Format("{0}({0:X})", Type);
		result["Size"] = string.Format("{0}({0:X})", Size);
		result["Data"] = Data;
//		result["Data"] = string.Join(" ", Enumerable.Range(0, Data.Length)
//			.Select(i => Data[i].ToString("X2")));
		return result.Select(x => x.Value);
    }   
	#endregion
}