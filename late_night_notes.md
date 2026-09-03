**LATE NIGHT NOTES**
----------------

Welcome one and all, I am your host benstor214 and this is the Late Night Notes.
We have great ideas coming up this evening so stay with us!
But before we get to our next segment, an empty line!

**Idea#1**
When epoll catches a read event on the child's stdout, we can use a wrapper function of the Buffer's very own fetchData() function, similar to the queueIncomingData() function for read events on the server socket belonging to the server-client TCP connection. ~~The request state during this will be CGI_PROCESSING. After that we can have the Dispatcher build an actual Response object.~~ The client received a third buffer '_pipestream' for that purpose.

**Idea#2**
I realised we don't need a separate state for CGI requests at this point. The state will be set to COMPLETE. The client state is set depending on the bool requires_CGI to either AWAITING_CGI_OUTPUT or PREPARING_RESPONSE. On AWAITING_CGI_OUTPUT the client will be called by the server when epoll catches a read event on the child's stdout.

**Idea#3**
The buffer will then be parsed by a yet-yo-be implemented CGI reponse parser. A converter will transform the CGI response a HTTP response.

**Idea#4**
I invited the famous hallucinator Claude to have him do a live presentation vibe-coding a CGI response parser and a CGItoHTTP-response converter.
They will be integrated into Gabi's CGI handler with sweat and blood,
so		much		blood! On that note:

The floggings will continue until morale improves!

**Idea#5**
Instead of separate parser and converter we have CGIResponse.cpp courtesy of Claude.
