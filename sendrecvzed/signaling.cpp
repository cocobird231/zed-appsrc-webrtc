#include <gst/gst.h>
#include <gst/webrtc/webrtc.h>
#include <json-glib/json-glib.h>  //json parsed

#include "appdata.h"
#include "signaling.h"
#include "pipeline.h"
#include "my_webrtc/negotiate.h"

static SoupWebsocketConnection *main_wsconn = NULL;

static void
Signaling_set_main_wsconnection(SoupWebsocketConnection *conn)
{
  main_wsconn = conn;
  g_object_ref(main_wsconn);
}

static void
Signaling_close_main_wsconnection(SoupWebsocketConnection *conn)
{
  g_object_unref(main_wsconn);
  main_wsconn = NULL;
}

static gboolean
Signaling_setup_call (const gchar * peer_id);

static gboolean
Signaling_register_with_server(const gchar *our_id);

static gboolean
cleanup_and_quit_loop (const gchar * msg, enum AppState state, MyAppData *myapp);

static void
on_server_connected (SoupSession * session, GAsyncResult * res,
    gpointer user_data);

static void
on_server_message (SoupWebsocketConnection * conn, SoupWebsocketDataType type,
    GBytes * message, gpointer user_data);

static void
on_server_closed (SoupWebsocketConnection * conn G_GNUC_UNUSED,
    gpointer user_data G_GNUC_UNUSED);

/*
 * Connect to the signalling server. This is the entrypoint for everything else.
 */
extern void
Siggnaling_connect_to_websocket_server_async (MyAppData * myapp)
{
  SoupLogger *logger;
  SoupMessage *message;
  SoupSession *session;
  const char *https_aliases[] = { "wss", NULL };

  session =
      soup_session_new_with_options (SOUP_SESSION_SSL_STRICT, FALSE,
      SOUP_SESSION_SSL_USE_SYSTEM_CA_FILE, TRUE,
      //SOUP_SESSION_SSL_CA_FILE, "/etc/ssl/certs/ca-bundle.crt",
      SOUP_SESSION_HTTPS_ALIASES, https_aliases, NULL);

  logger = soup_logger_new (SOUP_LOGGER_LOG_BODY, -1);
  soup_session_add_feature (session, SOUP_SESSION_FEATURE (logger));
  g_object_unref (logger);

  message = soup_message_new (SOUP_METHOD_GET, get_server_url(myapp));

  g_print ("Connecting to server...\n");

  /* Once connected, we will register */
  soup_session_websocket_connect_async (session, message, NULL, NULL, NULL, (GAsyncReadyCallback) on_server_connected, myapp);
  //soup_session_websocket_connect_async (session, message, NULL, NULL, NULL, (GAsyncReadyCallback) on_server_connected, [my call back function]);//callback方法
  set_appstate(SERVER_CONNECTING);
}

/*server connected
 *3 step
  1:set signal: closed
  2:set signal: message
  3:Signaling_register_with_server ()
 */
static void
on_server_connected (SoupSession * session, GAsyncResult * res,
    gpointer user_data)
{
  MyAppData *myapp = (MyAppData *) user_data;
  SoupWebsocketConnection *conn;
  GError *error = NULL;

  conn = soup_session_websocket_connect_finish (session, res, &error);
  myapp->ws_conn = conn;
  Signaling_set_main_wsconnection(conn);

  if (error) {
    cleanup_and_quit_loop (error->message, SERVER_CONNECTION_ERROR, myapp);
    g_error_free (error);
    return;
  }

  g_assert_nonnull (conn);
  
  set_appstate(SERVER_CONNECTED);
  g_print ("Connected to signalling server\n");

  g_signal_connect (main_wsconn, "closed", G_CALLBACK (on_server_closed), myapp);
  g_signal_connect (main_wsconn, "message", G_CALLBACK (on_server_message), myapp);

  //Register with the server so it knows about us and can accept commands 
  gboolean registered = FALSE;
  while(!registered){
    registered = Signaling_register_with_server (get_our_id(myapp));
  }
}

static void
on_server_closed (SoupWebsocketConnection * conn ,
    gpointer user_data)
{
  MyAppData *myapp = (MyAppData *) user_data;
  /*state change*/
  set_appstate(SERVER_CLOSED);

  cleanup_and_quit_loop ("Server connection closed", (AppState)0, myapp);
}

/* One mega message handler for our asynchronous calling mechanism */
static void
on_server_message (SoupWebsocketConnection * conn, SoupWebsocketDataType type,
    GBytes * message, gpointer user_data)
{
  MyAppData *myapp = (MyAppData *) user_data;
  gchar *text;

  switch (type) {
    case SOUP_WEBSOCKET_DATA_BINARY:
      g_printerr ("Received unknown binary message, ignoring\n");
      return;
    case SOUP_WEBSOCKET_DATA_TEXT:{
      gsize size;
      const gchar *data = (const gchar *)g_bytes_get_data(message, &size);
      /* Convert to NULL-terminated string */
      text = g_strndup (data, size);
      break;
    }
    default:
      g_assert_not_reached ();//exit program
  }

  /* Server has accepted our registration, we are ready to send commands */
  if (g_strcmp0 (text, "HELLO") == 0) {
    if (get_appstate() != SERVER_REGISTERING) {
      cleanup_and_quit_loop ("ERROR: Received HELLO when not registering",
          APP_STATE_ERROR, myapp);
      goto out;
    }
    set_appstate(SERVER_REGISTERED);
    g_print ("Registered with server\n");

    if( get_remote_is_offerer(myapp) ){/*wait sdp offer*/
      //do not thing
      set_appstate(PEER_CALL_NEGOTIATING);
      if (!start_pipeline (myapp->gstdata, get_remote_is_offerer(myapp) ))
      cleanup_and_quit_loop ("ERROR: failed to start pipeline, i am recv",
          PEER_CALL_NEGOTIATING, myapp);
    }else{/* Ask signalling server to connect us with a specific peer */
      if (!Signaling_setup_call (get_peer_id(myapp))) {
        cleanup_and_quit_loop ("ERROR: Failed to setup call", PEER_CALL_ERROR, myapp);
        goto out;
      }
    }
    
  } else if (g_strcmp0 (text, "SESSION_OK") == 0) {/* Call has been setup by the server, now we can start negotiation */
    if (get_appstate() != PEER_CONNECTING) {
      cleanup_and_quit_loop ("ERROR: Received SESSION_OK when not calling",
          PEER_CONNECTION_ERROR, myapp);
      goto out;
    }

    set_appstate(PEER_CONNECTED);
    /* Start negotiation (exchange SDP and ICE candidates) */
    if (!start_pipeline (myapp->gstdata, get_remote_is_offerer(myapp)))
      cleanup_and_quit_loop ("ERROR: failed to start pipeline",
          PEER_CALL_ERROR, myapp);
    /* Handle errors */
  } else if (g_str_has_prefix (text, "ERROR")) {
    switch (get_appstate()) {
      case SERVER_CONNECTING:
        set_appstate(SERVER_CONNECTION_ERROR);
        break;
      case SERVER_REGISTERING:
        set_appstate(SERVER_REGISTRATION_ERROR);
        break;
      case PEER_CONNECTING:
        set_appstate(PEER_CONNECTION_ERROR);
        break;
      case PEER_CONNECTED:
      case PEER_CALL_NEGOTIATING:
        set_appstate(PEER_CALL_ERROR);
        break;
      default:
        set_appstate(APP_STATE_ERROR);
    }
    cleanup_and_quit_loop (text, (AppState)0, myapp);
  } else {/* Look for JSON messages containing SDP and ICE candidates */
    JsonNode *root;
    JsonObject *object, *child;
    JsonParser *parser = json_parser_new ();
    if (!json_parser_load_from_data (parser, text, -1, NULL)) {
      g_printerr ("Unknown message '%s', ignoring", text);
      g_object_unref (parser);
      goto out;
    }

    root = json_parser_get_root (parser);
    if (!JSON_NODE_HOLDS_OBJECT (root)) {
      g_printerr ("Unknown json message '%s', ignoring", text);
      g_object_unref (parser);
      goto out;
    }

    object = json_node_get_object (root);
    /* Check type of JSON message */
    if (json_object_has_member (object, "sdp")) {
      int ret;
      GstSDPMessage *sdp;
      const gchar *text, *sdptype;
      GstWebRTCSessionDescription *answer;

      g_assert_cmphex (get_appstate(), ==, PEER_CALL_NEGOTIATING);

      child = json_object_get_object_member (object, "sdp");

      if (!json_object_has_member (child, "type")) {
        cleanup_and_quit_loop ("ERROR: received SDP without 'type'",
            PEER_CALL_ERROR, myapp);
        goto out;
      }

      sdptype = json_object_get_string_member (child, "type");
      /* In this example, we create the offer and receive one answer by default,
       * but it's possible to comment out the offer creation and wait for an offer
       * instead, so we handle either here.
       *
       * See tests/examples/webrtcbidirectional.c in gst-plugins-bad for another
       * example how to handle offers from peers and reply with answers using webrtcbin. */
      text = json_object_get_string_member (child, "sdp");
      ret = gst_sdp_message_new (&sdp);
      g_assert_cmphex (ret, ==, GST_SDP_OK);
      ret = gst_sdp_message_parse_buffer ((guint8 *) text, strlen (text), sdp);
      g_assert_cmphex (ret, ==, GST_SDP_OK);

      if (g_str_equal (sdptype, "answer")) {
        g_print ("Received answer:\n%s\n", text);
        answer_received (sdp, myapp->gstdata->webrtc1);
        set_appstate(PEER_CALL_STARTED);
        
      } else {
        g_print ("Received offer:\n%s\n", text);
        offer_received (sdp, myapp->gstdata->webrtc1);
      }

    } else if (json_object_has_member (object, "ice")) {
      const gchar *candidate;
      gint sdpmlineindex;

      child = json_object_get_object_member (object, "ice");
      candidate = json_object_get_string_member (child, "candidate");
      sdpmlineindex = json_object_get_int_member (child, "sdpMLineIndex");

      /* Add ice candidate which sent by remote peer to the webrtcbin*/
      g_signal_emit_by_name (myapp->gstdata->webrtc1, "add-ice-candidate", sdpmlineindex,
          candidate);
    } else {
      g_printerr ("Ignoring unknown JSON message:\n%s\n", text);
    }
    g_object_unref (parser);
  }

out:
  g_free (text);
}

static gboolean
Signaling_register_with_server (const gchar *our_id)
{
  if (soup_websocket_connection_get_state (main_wsconn) !=
      SOUP_WEBSOCKET_STATE_OPEN)
    return FALSE;

  g_print ("Registering id %s with server\n", our_id);
  set_appstate(SERVER_REGISTERING);

  /* Register with the server with a random integer id. Reply will be received
   * by on_server_message() */
  Signaling_send_register(our_id);

  return TRUE;
}

static gboolean
Signaling_setup_call (const gchar *peer_id)
{
  if (soup_websocket_connection_get_state (main_wsconn) !=
      SOUP_WEBSOCKET_STATE_OPEN)
    return FALSE;

  if (!peer_id)
    return FALSE;

  g_print ("Setting up signalling server call with %s\n", peer_id);
  set_appstate(PEER_CONNECTING);
  Signaling_send_SESSION(peer_id);
  return TRUE;
}

static gboolean
cleanup_and_quit_loop (const gchar * msg, enum AppState state, MyAppData *myapp)
{
  if (msg)
    g_printerr ("%s\n", msg);
  if (state > 0)
    set_appstate(state);

  if (main_wsconn) {
    if (soup_websocket_connection_get_state (main_wsconn) ==
        SOUP_WEBSOCKET_STATE_OPEN)
      /* This will call us again */
      soup_websocket_connection_close (main_wsconn, 1000, "");
    else
      g_object_unref (main_wsconn);
      main_wsconn = NULL;
  }

  if (myapp->main_loop) {
    g_main_loop_quit (myapp->main_loop);
  }

  /* To allow usage as a GSourceFunc */
  return G_SOURCE_REMOVE;
}

/*signaling transport*/
extern gchar *
get_string_from_json_object (JsonObject * object)
{
  JsonNode *root;
  JsonGenerator *generator;
  gchar *text;

  /* Make it the root node */
  root = json_node_init_object (json_node_alloc (), object);
  generator = json_generator_new ();
  json_generator_set_root (generator, root);
  text = json_generator_to_data (generator, NULL);

  /* Release everything */
  g_object_unref (generator);
  json_node_free (root);
  return text;
}

extern void
Signaling_send_register(const gchar *my_id)
{
  gchar *hello;
  hello = g_strdup_printf ("HELLO %s", my_id);
  soup_websocket_connection_send_text (main_wsconn, hello);
  g_free (hello);
}

extern void
Signaling_send_sdp_to_peer (GstWebRTCSessionDescription * desc)
{
  gchar *text;
  JsonObject *msg, *sdp;
/*
  if (get_appstate() < PEER_CALL_NEGOTIATING) {
    cleanup_and_quit_loop ("Can't send SDP to peer, not in call",
        APP_STATE_ERROR, myapp);
    return;
  }
*/
  text = gst_sdp_message_as_text (desc->sdp);
  sdp = json_object_new ();
    /*set sdp type :offer/answer*/
  if (desc->type == GST_WEBRTC_SDP_TYPE_OFFER) {
    g_print ("Sending offer:\n%s\n", text);
    json_object_set_string_member (sdp, "type", "offer");
  } else if (desc->type == GST_WEBRTC_SDP_TYPE_ANSWER) {
    g_print ("Sending answer:\n%s\n", text);
    json_object_set_string_member (sdp, "type", "answer");
  } else {
    g_assert_not_reached ();
  }

  json_object_set_string_member (sdp, "sdp", text);
  g_free (text);

  msg = json_object_new ();
  json_object_set_object_member (msg, "sdp", sdp);
  text = get_string_from_json_object (msg);
  json_object_unref (msg);

  soup_websocket_connection_send_text (main_wsconn, text);
  g_free (text);
}

extern void
Signaling_send_ice_candidate_message (GstElement * webrtc , guint mlineindex,
    gchar * candidate, gpointer user_data )
{
  //g_print("Signaling send ice candidate message\n");
  gchar *text;
  JsonObject *ice, *msg;
/*
  if (get_appstate() < PEER_CALL_NEGOTIATING) {
    cleanup_and_quit_loop ("Can't send ICE, not in call", APP_STATE_ERROR, myapp);
    return;
  }
*/
  ice = json_object_new ();
  json_object_set_string_member (ice, "candidate", candidate);
  json_object_set_int_member (ice, "sdpMLineIndex", mlineindex);
  msg = json_object_new ();
  json_object_set_object_member (msg, "ice", ice);
  text = get_string_from_json_object (msg);
  json_object_unref (msg);

  soup_websocket_connection_send_text (main_wsconn, text);
  g_free (text);
}

extern void
Signaling_send_OFFER_REQUEST_to_peer()
{
    gchar *msg = g_strdup_printf ("OFFER_REQUEST");
    soup_websocket_connection_send_text (main_wsconn, msg);
    g_free (msg);
}

extern void 
Signaling_send_SESSION(const gchar *peer_id)
{
  gchar *msg = g_strdup_printf ("SESSION %s", peer_id);
  soup_websocket_connection_send_text (main_wsconn, msg);
  g_free (msg);
}