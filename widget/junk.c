



    if (serv->option_range && iprange_scan (option_range, (const unsigned char *) &client_address.sin_addr.s_addr, &found_ip)) {
        printf ("range error: %s\n", option_range);
        exit (1);
    }






    sock = accept (h, (struct sockaddr *) &client_address, &l);
    if (sock < 0) {
        printf ("accept fail\n");
        goto restart;
    }
    if (!option_range) {
        found_ip = 1;
    } else {
        iprange_scan (option_range, (const unsigned char *) &client_address.sin_addr.s_addr, &found_ip);
    }

    if (!found_ip) {
        SHUTSOCK (sock);
        printf ("incoming address not in range %s\n", option_range);
        goto restart;
    }

    {
        int yes = 1;
        if (setsockopt (sock, IPPROTO_TCP, TCP_NODELAY, (char *) &yes, sizeof (yes))) {
            perror ("setsockopt TCP_NODELAY\n");
            exit (1);
        }
        if (fcntl (sock, F_SETFL, O_NONBLOCK)) {
            perror ("fcntl O_NONBLOCK\n");
            exit (1);
        }
    }

    printf ("connection established\n");

    memset (&d, '\0', sizeof (d));
    d.sock = sock;

    memset (&sd, '\0', sizeof (sd));
    sd.reader_data = &d;




    for (;;) {
        CStr r;
        unsigned char *p;
        unsigned long long msglen;
        unsigned long version, action, magic;
        struct cooledit_remote_msg_header m;

        if (!wait_for_read (sock, 1000))
            continue;

        if (reader (&d, &m, sizeof (m)))
            goto restart;

        decode_msg_header (&m, &msglen, &version, &action, &magic);

        if (magic != FILE_PROTO_MAGIC) {
            printf ("bad magic, closing\n");
            goto restart;
        }

        if (msglen > 1024 * 1024) {
            printf ("msglen < 1M, closing\n");
            goto restart;
        }

        p = (unsigned char *) malloc (msglen);

        if (reader (&d, p, msglen)) {
            free (p);
            goto restart;
        }
        memset (&r, '\0', sizeof (r));

        if (action >= 1 && action < sizeof (action_list) / sizeof (action_list[0])) {
            if ((*action_list[action].action_fn) (&sd, &r, p, msglen)) {
                free (p);
                goto restart;
            }
        } else {
            if ((*action_list[REMOTEFS_ACTION_NOTIMPLEMENTED].action_fn) (&sd, &r, p, msglen)) {
                free (p);
                goto restart;
            }
        }

        free (p);

        encode_msg_header (&m, r.len, MSG_VERSION, action, FILE_PROTO_MAGIC);
        if (writer (sock, &m, sizeof (m))) {
            free (r.data);
            goto restart;
        }
        if (writer (sock, r.data, r.len)) {
            free (r.data);
            goto restart;
        }
        free (r.data);
    }
