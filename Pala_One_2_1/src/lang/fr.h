#ifndef PALA_LANG_FR_H
#define PALA_LANG_FR_H

// ============================================================================
//  French string table — fr.
//  Mirror of en.h; the key set MUST stay identical. Adding new keys: edit en.h
//  first, then add the same key here. See src/lang/lang.h for the rule.
//
//  Glyph coverage: all accents used here (à â ç é è ê ë î ï ô ù û ü œ) are in
//  u8g2's Latin Extended (_te) font set already linked by src/ui/font.cpp.
//
//  Apostrophes:
//    - On-device strings (non-D_WEB_*) use the ASCII ' — the typographic ’
//      (U+2019) is outside the _te glyph range and would not render.
//    - Web strings (D_WEB_*) use the typographic ’ (U+2019), never the ASCII
//      ' — the web UI emits most strings inside single-quoted HTML attributes
//      and JS literals (placeholder='…', confirm('…'), status strings), where
//      ' breaks parsing.
// ============================================================================

// ----------------------------------------------------------------------------
//  Boot / fatal screens (Pala_One_2_1.ino)
// ----------------------------------------------------------------------------
#define D_BOOT_STORAGE_ERROR        "Erreur de stockage"
#define D_BOOT_TRY_FACTORY_RESET    "Essayez une réinitialisation"

// ----------------------------------------------------------------------------
//  About screen (src/ui/screens/about_screen.cpp)
// ----------------------------------------------------------------------------
#define D_ABOUT_HEADER              "Appareil"
#define D_ABOUT_FIRMWARE_PREFIX     "Firmware"
#define D_ABOUT_GESTURE_CLICK       "clic"
#define D_ABOUT_GESTURE_CLICK_2     "clic x 2"
#define D_ABOUT_GESTURE_CLICK_3     "clic x 3"
#define D_ABOUT_GESTURE_CLICK_HOLD  "clic + maintien"
#define D_ABOUT_GESTURE_HOLD        "maintien"
#define D_ABOUT_GESTURE_LONG_HOLD   "maintien long"
#define D_ABOUT_GESTURE_SEPARATOR   " : "

// ----------------------------------------------------------------------------
// Action names  (src/ui/reader_actions.cpp)
// ----------------------------------------------------------------------------
#define D_ACTION_NONE_LABEL     "Aucune"
#define D_ACTION_NEXT_LABEL     "Suivant"
#define D_ACTION_OPEN_LABEL     "Ouvrir/Choisir"
#define D_ACTION_BOOKMARK_LABEL "Marque-page"
#define D_ACTION_HOME_LABEL     "Accueil"
#define D_ACTION_LOCK_LABEL     "Verrouiller"
#define D_ACTION_MENU_LABEL     "Menu"
#define D_ACTION_ROTATE_LABEL   "Retourner écran"

// ----------------------------------------------------------------------------
//  Library screen — section title + system menu entries
//  (src/ui/screens/library_screen.cpp). The "+ " / "- " expansion indicators
//  in entryLabel() are visual symbols and intentionally NOT translated.
// ----------------------------------------------------------------------------
#define D_MENU_BOOKMARKS            "Marque-pages"
#define D_MENU_LIST                 "Liste"
#define D_MENU_SETTINGS             "Mise en page"
#define D_MENU_APPS                 "Apps"
#define D_MENU_STATISTICS           "Statistiques"
#define D_MENU_DEVICE               "Appareil"
#define D_MENU_UPLOAD               "Connexion"
#define D_LIBRARY_OPEN_FAILED       "Ouverture impossible"
#define D_LIBRARY_TRY_UPLOAD        "Réessayez le transfert"

// ----------------------------------------------------------------------------
//  Statistics screen (src/ui/screens/statistics_screen.cpp). The *_FMT
//  strings are snprintf templates with %u / %llu placeholders — keep the
//  positional order across translations.
// ----------------------------------------------------------------------------
#define D_STATS_HEADING                "Statistiques"
#define D_STATS_STREAK_CURRENT_FMT     "Série actuelle : %u jours"
#define D_STATS_STREAK_LONGEST_FMT     "Record : %u  Sessions : %u"
#define D_STATS_LIFETIME_PAGES_FMT     "Pages tournées : %llu"
#define D_STATS_LIFETIME_PRESSES_FMT   "Appuis bouton : %llu"

// ----------------------------------------------------------------------------
//  List screen (src/ui/screens/list_screen.cpp)
// ----------------------------------------------------------------------------
#define D_LIST_HEADER               "Liste"
#define D_LIST_NONE                 "Aucun élément"

// ----------------------------------------------------------------------------
//  Settings screen (src/ui/screens/settings_screen.cpp). For the most part we
//  reuse the same labels as the Web UI, but a few we need to redefine due to
//  embedded html elements.
// ----------------------------------------------------------------------------
#define D_SETTINGS_HEADER           "Mise en page"

#define D_SETTINGS_GAP_0            "0 - serré"
#define D_SETTINGS_GAP_1            "1 - normal"
#define D_SETTINGS_GAP_2            "2 - aéré"
#define D_SETTINGS_GAP_3            "3 - très aéré"

#define D_SETTINGS_SIZE_8           "8 - très petite"
#define D_SETTINGS_SIZE_10          "10 - petite"
#define D_SETTINGS_SIZE_12          "12 - moyenne"
#define D_SETTINGS_SIZE_14          "14 - grande"

// ----------------------------------------------------------------------------
//  Upload screen (src/ui/screens/upload_screen.cpp)
// ----------------------------------------------------------------------------
#define D_UPLOAD_HEADER             "Connexion"
#define D_UPLOAD_WIFI               "Wi-Fi"
#define D_UPLOAD_PASSWORD           "Mot de passe"
#define D_UPLOAD_OPEN               "Ouvrir"
#define D_UPLOAD_CONNECTING         "Connexion en cours"
#define D_UPLOAD_CONNECTED          "Connecté"
#define D_UPLOAD_HOTSPOT_HINT_L1    "Appuyez sur le bouton"
#define D_UPLOAD_HOTSPOT_HINT_L2    "pour le point d'accès"

// ----------------------------------------------------------------------------
//  Apps screen (src/ui/screens/apps_screen.cpp)
// ----------------------------------------------------------------------------
#define D_APPS_HEADER               "Apps"
#define D_APPS_NONE                 "Aucune app installée"

// ----------------------------------------------------------------------------
//  Update screen (src/ui/screens/update_screen.cpp)
// ----------------------------------------------------------------------------
#define D_MENU_UPDATE               "Mise à jour FW"
#define D_UPDATE_HEADER             "Mise à jour FW"
#define D_UPDATE_VERSION_PREFIX     "Version actuelle : "
#define D_UPDATE_CHANNEL_LABEL      "Canal du firmware :"
#define D_UPDATE_CHAN_STABLE        "Stable"
#define D_UPDATE_CHAN_DEV           "Dev"
#define D_UPDATE_BTN_CHECK          "[ Rechercher une MàJ ]"
#define D_UPDATE_NO_CREDS_L1        "Aucun identifiant Wi-Fi."
#define D_UPDATE_NO_CREDS_L2        "Configurez via le site web."
#define D_UPDATE_CONNECTING         "Connexion..."
#define D_UPDATE_CONN_FAILED        "Échec de connexion Wi-Fi"
#define D_UPDATE_CHECKING           "Vérification..."
#define D_UPDATE_SERVER_FAIL        "Serveur injoignable"
#define D_UPDATE_UP_TO_DATE         "Déjà à jour"
#define D_UPDATE_AVAILABLE_PREFIX   "Disponible : "
#define D_UPDATE_BTN_INSTALL        "[ Installer la MàJ ]"
#define D_UPDATE_INSTALLING         "Installation..."
#define D_UPDATE_DOWNLOAD_FAILED    "Échec du téléchargement"
#define D_UPDATE_REBOOT_MSG         "Mise à jour installée"
#define D_UPDATE_REBOOT_HINT        "2x pour redémarrer"

// ----------------------------------------------------------------------------
//  Bookmarks screens
//  (src/ui/screens/bookmarks/{book_select_screen,bookmark_list_screen}.cpp)
// ----------------------------------------------------------------------------
#define D_BOOKMARKS_HEADER          "Marque-pages"
#define D_BOOKMARKS_NO_BOOKS        "Aucun livre"
#define D_BOOKMARKS_NONE            "Aucun marque-page"
#define D_BOOKMARKS_OPEN_FAILED     "Ouverture impossible"

// ----------------------------------------------------------------------------
//  Reader (src/ui/reader.cpp)
// ----------------------------------------------------------------------------
#define D_READER_BOOK_EMPTY         "Livre vide"
#define D_READER_BACK_LIBRARY       "Retour à la bibliothèque"
#define D_READER_INDEXING_TITLE     "Préparation du livre..."
#define D_READER_INDEXING_DETAIL    "Reconstruction de l'index"

// ----------------------------------------------------------------------------
//  Reader menu overlay (src/ui/reader_menu.cpp). Both *_FMT strings are
//  snprintf templates rendered into a char[64]; keep the argument order —
//  PAGE_OF_FMT takes (page, total, percent), PAGE_PCT_FMT takes (page,
//  percent). STATUSBAR_FMT's %s is one of the D_STATUSBAR_MODE_* labels.
// ----------------------------------------------------------------------------
#define D_READER_MENU_HEADING       "Lecture"
#define D_READER_MENU_PAGE_OF_FMT   "Page %d sur %d  (%d%%)"
#define D_READER_MENU_PAGE_PCT_FMT  "Page %d  (%d%% du livre)"
#define D_READER_MENU_STATUSBAR_FMT "Barre d'état : %s"
#define D_READER_MENU_HINT          "clic : changer  2x : fermer"
#define D_STATUSBAR_MODE_FULL       "Complète"
#define D_STATUSBAR_MODE_MINIMAL    "Minimale"
#define D_STATUSBAR_MODE_HIDDEN     "Masquée"

// ----------------------------------------------------------------------------
//  Toasts (src/ui/screen_settings.cpp, src/storage/statistics.cpp)
// ----------------------------------------------------------------------------
#define D_TOAST_SCREEN_FLIPPED      "Écran retourné"
#define D_TOAST_STREAK_DAY_FMT      "Série de lecture : jour %u"


// ----------------------------------------------------------------------------
//  App loader error overlay (src/ui/pala_api_impl.cpp paintLoadError)
// ----------------------------------------------------------------------------
#define D_APP_ERR_TITLE             "Erreur app"
#define D_APP_ERR_NULL_PATH         "chemin nul"
#define D_APP_ERR_NOT_FOUND         "App introuvable"
#define D_APP_ERR_TOO_SMALL         "App trop petite"
#define D_APP_ERR_INVALID_FILE      "Fichier invalide"
#define D_APP_ERR_TOO_LARGE         "App trop grande"
#define D_APP_ERR_SIZE_LIMIT        "> 48 o"
#define D_APP_ERR_READ              "Erreur de lecture"
#define D_APP_ERR_PARTIAL_READ      "Lecture partielle"
#define D_APP_ERR_NO_EXEC_MEM       "Mémoire exec insuffisante"
#define D_APP_ERR_BAD_FILE          "Fichier app invalide"
#define D_APP_ERR_WRONG_MAGIC       "Signature invalide"
#define D_APP_ERR_API_MISMATCH      "API incompatible"
#define D_APP_ERR_API_FMT           "API v%u, requiert v%u"
#define D_APP_ERR_BAD_ENTRY         "Point d'entrée invalide"
#define D_APP_ERR_BAD_RELOC         "Table reloc invalide"
#define D_APP_ERR_RELOC_RANGE       "Reloc hors limites"

// ----------------------------------------------------------------------------
//  Bookmark add toasts (src/pure/bookmarks_codec.cpp)
//  These are pointers returned from a pure module and rendered via Toast::show
//  in reader_screen.cpp.
// ----------------------------------------------------------------------------
#define D_TOAST_BOOKMARK_EXISTS     "Marque-page existant"
#define D_TOAST_BOOKMARK_SAVED      "Marque-page ajouté"

// ----------------------------------------------------------------------------
//  Lock / screensaver (src/ui/sleep.cpp, Pala_One_2_1.ino)
// ----------------------------------------------------------------------------
#define D_SCREENSAVER_LOCKED        "Verrouillé"
#define D_TOAST_UNLOCKED            "Déverrouillé"

// ============================================================================
//  Web UI (captive portal) — strings embedded in HTML via adjacent-literal
//  concatenation. All endpoints declare Content-Type: charset=utf-8 already,
//  so accented characters survive transit unchanged.
// ============================================================================

// ----------------------------------------------------------------------------
//  Shared chrome / storage card (src/web/chrome.{h,cpp})
// ----------------------------------------------------------------------------
#define D_WEB_STORAGE_HEADING       "Stockage"
#define D_WEB_STORAGE_BOOKS         "Livres"
#define D_WEB_STORAGE_USED          "Utilisé"
#define D_WEB_STORAGE_FREE          "Libre"
#define D_WEB_STORAGE_TOTAL         "Total"
#define D_WEB_STORAGE_PCT_SUFFIX    " % du stockage interne actuellement utilisé."

// ----------------------------------------------------------------------------
//  Navigation links (used across multiple route handlers)
// ----------------------------------------------------------------------------
#define D_WEB_NAV_HOME              "Accueil"
#define D_WEB_NAV_FILES             "Fichiers"
#define D_WEB_NAV_BOOKMARKS         "Marque-pages"
#define D_WEB_NAV_LIST              "Liste"
#define D_WEB_NAV_SCREENSAVER       "Écran de veille"
#define D_WEB_NAV_SETTINGS          "Réglages"
#define D_WEB_NAV_FACTORY_RESET     "Réinitialisation"
#define D_WEB_NAV_BACK              "Retour"

// ----------------------------------------------------------------------------
//  Theme toggle in the page header (src/web/chrome.cpp webPageStart). The
//  title is emitted inside a single-quoted HTML attribute, so it follows the
//  no-apostrophe rule from lang.h.
// ----------------------------------------------------------------------------
#define D_WEB_THEME_TOGGLE_TITLE    "Apparence claire ou sombre"
#define D_WEB_THEME_DARK_MODE       "Mode sombre"
#define D_WEB_THEME_LIGHT_MODE      "Mode clair"

// ----------------------------------------------------------------------------
//  Home page (src/web/files.cpp handleRoot)
// ----------------------------------------------------------------------------
#define D_WEB_HOME_TITLE            "Pala One"
#define D_WEB_HOME_FW_PREFIX        "Firmware "
#define D_WEB_HOME_MIDDOT_SEP       " &middot; "
#define D_WEB_HOME_BOOKS_SUFFIX     " livres"
#define D_WEB_HOME_FREE_LABEL       "Libre : "
#define D_WEB_HOME_STORAGE_WARN     "&#9888; Le stockage est indisponible ou presque plein. Si les transferts échouent, supprimez des livres ou utilisez la réinitialisation depuis cette interface web."
#define D_WEB_UPLOAD_BOOK_HEADING   "Ajouter un livre"
#define D_WEB_UPLOAD_BOOK_DESC      "Envoyez des fichiers texte brut UTF-8 vers <b>/books</b> sur l’appareil, puis rangez-les dans des dossiers depuis la page Fichiers."
#define D_WEB_UPLOAD_BOOK_BUTTON    "Envoyer"
#define D_WEB_MANAGE_FILES_BUTTON   "Gérer les fichiers"
#define D_WEB_INSTALL_APP_HEADING   "Installer une app"
#define D_WEB_INSTALL_APP_DESC      "Envoyez un binaire d’app Pala (<b>.bin</b>) vers <b>/apps</b>. L’en-tête est vérifié avant l’enregistrement ; seuls les fichiers avec la bonne signature et la bonne version d’API sont acceptés. Ouvrez <b>Apps</b> depuis la bibliothèque pour la lancer."
#define D_WEB_INSTALL_APP_BUTTON    "Installer l’app"
#define D_WEB_NOTES_HEADING         "Remarques"
#define D_WEB_NOTES_DESC            "Les livres envoyés sont normalisés et compactés avant l’enregistrement : un fichier TXT source peut donc être plus gros que le fichier final stocké. Le lecteur est optimisé pour le texte brut UTF-8 et les langues à alphabet latin."

// ----------------------------------------------------------------------------
//  Files page (src/web/files.cpp handleFiles + folder/move/jump forms)
// ----------------------------------------------------------------------------
#define D_WEB_FILES_HEADING         "Fichiers"
#define D_WEB_FILES_SUBTITLE        "Gérez les livres, les dossiers et l’organisation de la bibliothèque du Pala One."
#define D_WEB_CREATE_FOLDER_HEADING "Créer un dossier"
#define D_WEB_CREATE_FOLDER_PLACEHOLDER "romans ou classiques/francais"
#define D_WEB_CREATE_FOLDER_BUTTON  "Créer le dossier"
#define D_WEB_CREATE_FOLDER_HINT    "Les dossiers se trouvent dans /books."
#define D_WEB_FOLDERS_HEADING       "Dossiers"
#define D_WEB_NO_FOLDERS            "Aucun dossier pour l’instant. Les livres sont à la racine de /books."
#define D_WEB_CONFIRM_DELETE_FOLDER "Supprimer le dossier ? Seuls les dossiers vides peuvent être supprimés."
#define D_WEB_DELETE_BUTTON         "Supprimer"
#define D_WEB_LIBRARY_FILES_HEADING "Fichiers de la bibliothèque"
#define D_WEB_LIBRARY_FULL_WARN     "&#9888; Bibliothèque pleine (80 livres max). Supprimez des livres pour libérer de la place."
#define D_WEB_FOLDER_LIMIT_WARN     "&#9888; Limite de dossiers atteinte (32 max)."
#define D_WEB_NO_BOOKS_UPLOADED     "Aucun livre envoyé pour l’instant."
#define D_WEB_BOOK_ROOT             "Racine"
#define D_WEB_BOOK_BYTES_LABEL      " octets"
#define D_WEB_BOOK_FOLDER_LABEL     " &middot; dossier : "
#define D_WEB_BOOK_CURRENT_PAGE     " &middot; page actuelle : "
#define D_WEB_JUMP_BUTTON           "Aller"
#define D_WEB_JUMP_HINT             "Définissez la page qui s’ouvrira ensuite sur l’appareil."
#define D_WEB_JUMP_HINT2            "La première ouverture peut prendre un moment."
#define D_WEB_PAGE_PLACEHOLDER      "Page"
#define D_WEB_MOVE_BUTTON           "Déplacer"
#define D_WEB_MOVE_HINT             "Utilisez le chemin exact du dossier."
#define D_WEB_MOVE_PLACEHOLDER      "laisser vide pour la racine"
#define D_WEB_CONFIRM_DELETE_FILE   "Supprimer le fichier ?"
#define D_WEB_DOWNLOAD_BUTTON       "Télécharger"
#define D_WEB_APPS_PAGE_HEADING     "Apps"
#define D_WEB_NO_APPS_INSTALLED     "Aucune app installée."
#define D_WEB_CONFIRM_DELETE_APP    "Supprimer l’app ?"

// ----------------------------------------------------------------------------
//  Plain-text 4xx/5xx error bodies (src/web/files.cpp, bookmarks.cpp,
//  apps_upload.cpp, find.cpp, screensavers.cpp). These reach the browser on
//  misformed requests; they surface as page content if the user navigates a
//  bad URL. The lowercase ones are argument-validation failures, the
//  capitalized ones are operation failures — keep that split in translation.
//  FILE_NAME_TOO_LONG's %d is MAX_FILE_NAME; %s is file name.
// ----------------------------------------------------------------------------
#define D_WEB_ERR_MISSING_ID            "id manquant"
#define D_WEB_ERR_BAD_ID                "id invalide"
#define D_WEB_ERR_MISSING_FOLDER        "dossier manquant"
#define D_WEB_ERR_BAD_FOLDER            "dossier invalide"
#define D_WEB_ERR_FOLDER_LIMIT          "limite de dossiers atteinte"
#define D_WEB_ERR_MKDIR_FAILED          "échec de mkdir"
#define D_WEB_ERR_FOLDER_NOT_FOUND      "dossier introuvable"
#define D_WEB_ERR_FOLDER_NOT_EMPTY      "dossier non vide"
#define D_WEB_ERR_DELETE_FAILED         "échec de la suppression"
#define D_WEB_ERR_FOLDER_CREATE_FAILED  "échec de la création du dossier"
#define D_WEB_ERR_DEST_EXISTS           "la destination existe déjà"
#define D_WEB_ERR_MOVE_FAILED           "échec du déplacement"
#define D_WEB_ERR_FILE_NAME_TOO_LONG    "échec du déplacement : le nouveau chemin dépasse %d caractères : %s"
#define D_WEB_ERR_MISSING_ID_PAGE       "id/page manquant"
#define D_WEB_ERR_MISSING_BOOK_IDX      "livre/idx manquant"
#define D_WEB_ERR_BAD_BOOK              "livre invalide"
#define D_WEB_ERR_BAD_IDX               "idx invalide"
#define D_WEB_ERR_MISSING_BOOK          "livre manquant"
#define D_WEB_ERR_MISSING_NAME          "nom manquant"
#define D_WEB_ERR_INVALID_NAME          "nom invalide"
#define D_WEB_ERR_MISSING_OFFSET        "décalage manquant"
#define D_WEB_ERR_OPEN_FAILED           "Ouverture impossible"
#define D_WEB_ERR_NOT_FOUND             "Introuvable"
#define D_WEB_ERR_APP_NOT_FOUND         "App introuvable"

// ----------------------------------------------------------------------------
//  List page (src/web/list.cpp)
// ----------------------------------------------------------------------------
#define D_WEB_LIST_HEADING          "Liste"
#define D_WEB_LIST_SUBTITLE         "Créez une simple liste de courses ou de tâches pour le Pala One."
#define D_WEB_LIST_EDIT_HEADING     "Modifier la liste"
#define D_WEB_LIST_EDIT_DESC        "Les éléments n’apparaissent sur l’appareil que si au moins une ligne contient du texte. Maintenez le bouton de l’appareil pour marquer un élément comme fait."
#define D_WEB_LIST_ITEM_PLACEHOLDER "Élément"
#define D_WEB_LIST_SAVE_BUTTON      "Enregistrer la liste"
#define D_WEB_LIST_DELETE_DONE      "Supprimer les éléments cochés"
#define D_WEB_LIST_HINT             "Les lignes vides sont ignorées. Les lignes cochées peuvent être supprimées directement."

// ----------------------------------------------------------------------------
//  Reset page (src/web/reset.cpp)
// ----------------------------------------------------------------------------
#define D_WEB_RESET_HEADING         "Réinitialisation"
#define D_WEB_RESET_SUBTITLE        "Effacer tous les livres, marque-pages, progressions et fichiers personnalisés."
#define D_WEB_RESET_CONFIRM_HEADING "Confirmer la réinitialisation"
#define D_WEB_RESET_WARNING         "Cela supprimera TOUS les livres, marque-pages et la progression de lecture."
#define D_WEB_RESET_DETAIL          "Le système de fichiers de l’appareil sera formaté et les réglages reviendront aux valeurs par défaut."
#define D_WEB_RESET_YES_BUTTON      "Oui, réinitialiser"
#define D_WEB_RESET_COMPLETE_HEADING "Réinitialisation terminée"
#define D_WEB_RESET_COMPLETE_DESC   "Tous les livres, marque-pages, progressions et fichiers personnalisés ont été supprimés. L’appareil est revenu à un état vierge."
#define D_WEB_GO_HOME_BUTTON        "Aller à l’accueil"
#define D_WEB_OPEN_FILES_BUTTON     "Ouvrir les fichiers"
#define D_WEB_RESET_SUCCESS_TITLE   "Réinitialisation terminée"
#define D_WEB_RESET_SUCCESS_SUBTITLE "Le Pala One a bien été réinitialisé."
#define D_WEB_RESET_BANNER          "&#10003; Réinitialisation terminée."

// ----------------------------------------------------------------------------
//  Settings page (src/web/settings.cpp)
// ----------------------------------------------------------------------------
#define D_WEB_SETTINGS_TITLE        "Réglages du Pala One"
#define D_WEB_SETTINGS_SUBTITLE_PREFIX "Firmware "
#define D_WEB_SETTINGS_SUBTITLE_SUFFIX " — page de configuration stockée directement sur l’appareil."
#define D_WEB_SETTINGS_BACK_NAV     "&#8592; Accueil"
#define D_WEB_READING_HEADING       "Lecture"
#define D_WEB_FONT_SIZE_LABEL       "Taille de police"
#define D_WEB_FONT_SIZE_8           "8px &mdash; très petite"
#define D_WEB_FONT_SIZE_10          "10px &mdash; petite"
#define D_WEB_FONT_SIZE_12          "12px &mdash; moyenne"
#define D_WEB_FONT_SIZE_14          "14px &mdash; grande"
#define D_WEB_FONT_SIZE_HINT        "Détermine le nombre de lignes par page."
#define D_WEB_SLEEP_AFTER_LABEL     "Veille après"
#define D_WEB_SLEEP_30S             "30 secondes"
#define D_WEB_SLEEP_1M              "1 minute"
#define D_WEB_SLEEP_2M              "2 minutes"
#define D_WEB_SLEEP_5M              "5 minutes"
#define D_WEB_SLEEP_10M             "10 minutes"
#define D_WEB_SLEEP_30M             "30 minutes"
#define D_WEB_SLEEP_HINT            "La mise en veille automatique limite la consommation de la batterie au repos."
#define D_WEB_LINE_SPACING_LABEL    "Interligne"
#define D_WEB_LINE_SPACING_0        "0 px &mdash; serré"
#define D_WEB_LINE_SPACING_1        "1 px &mdash; normal"
#define D_WEB_LINE_SPACING_2        "2 px &mdash; aéré"
#define D_WEB_LINE_SPACING_3        "3 px &mdash; très aéré"
#define D_WEB_LINE_SPACING_HINT     "Un petit changement ici peut rendre le texte bien plus facile à parcourir."
#define D_WEB_NO_SCREENSAVER_LABEL  "Mode sans écran de veille"
#define D_WEB_NO_SCREENSAVER_HINT   "L’appareil se met toujours en veille selon le délai normal et rafraîchit l’écran avant de s’endormir. Il affiche alors la dernière page du livre et évite un rafraîchissement complet au réveil : vous reprenez la lecture d’un simple clic, sans interruption due au rafraîchissement de l’écran."
#define D_WEB_LOCK_ON_SLEEP_LABEL   "Verrouiller à la mise en veille"
#define D_WEB_LOCK_ON_SLEEP_HINT    "Verrouille automatiquement l’appareil à chaque mise en veille. Un appui long sera nécessaire pour le déverrouiller au prochain réveil."
#define D_WEB_SAVE_SETTINGS_BUTTON  "Enregistrer les réglages"
#define D_WEB_LIBRARY_ORDER_HEADING  "Menu de la bibliothèque"
#define D_WEB_LIBRARY_ORDER_INTRO    "Choisissez les éléments affichés dans la bibliothèque et faites-les glisser pour les mettre dans l’ordre voulu. Les éléments masqués n’apparaissent pas du tout."
#define D_WEB_LIBRARY_ORDER_REQUIRED  D_MENU_UPLOAD " est obligatoire et restera toujours dans la bibliothèque."
#define D_WEB_LIBRARY_ORDER_SLOT_LABEL "Position"
#define D_WEB_LIBRARY_ORDER_HIDDEN   "Masqué"
#define D_WEB_LIBRARY_ORDER_RESET    "Rétablir l’ordre par défaut"
#define D_WEB_LIBRARY_ORDER_HINT     "L’écran utilise cet ordre immédiatement et le conserve après un redémarrage."
#define D_WEB_SETTINGS_NO_EXTRAS    "Aucun fichier, script ou police externe."
#define D_WEB_SCREENSAVER_HEADING   "Écran de veille"
#define D_WEB_SCREENSAVER_SPECS     "Envoyez des octets XBM bruts : <b>3904 octets</b>, 250&times;122 px, 1 bit, LSB en premier, 32 octets par ligne."
#define D_WEB_SCREENSAVER_TIP       "Astuce : utilisez <a class='link' href='https://javl.github.io/image2cpp/' target='_blank'>image2cpp</a> avec <b>Plain bytes</b>. Inversez les couleurs si besoin."
#define D_WEB_SCREENSAVER_ACTIVE    "&#10003; Écran de veille personnalisé actif."
#define D_WEB_CONFIRM_DEL_SCREENSAVER "Supprimer l’écran de veille personnalisé ?"
#define D_WEB_SCREENSAVER_DEFAULT   "Écran de veille intégré utilisé."
#define D_WEB_SLEEP_IMAGE_LABEL     "Fichier image de veille"
#define D_WEB_SCREENSAVER_UPLOAD_BUTTON "Envoyer l’image"
#define D_WEB_MISSING_REQUIRED_BUTTON_MSG "Ces actions doivent toujours être présentes : " D_WEB_BUTTONS_ACTION_HOME ", " D_WEB_BUTTONS_ACTION_NEXT ", " D_WEB_BUTTONS_ACTION_MENU

// Buttons / remappable hold-gestures section.
#define D_WEB_BUTTONS_HEADING       "Bouton"
#define D_WEB_BUTTONS_HINT          "Vous pouvez associer chaque type de clic à une action différente."
#define D_WEB_BUTTONS_LONG          "Appui long"
#define D_WEB_BUTTONS_SHORT         "Clic simple"
#define D_WEB_BUTTONS_DOUBLE        "Double clic"
#define D_WEB_BUTTONS_TRIPLE        "Triple clic"
#define D_WEB_BUTTONS_EXTRA_LONG    "Appui très long"
#define D_WEB_BUTTONS_CLICK_HOLD    "Clic, puis maintien"
#define D_WEB_BUTTONS_SAVE          "Enregistrer le bouton"
#define D_WEB_BUTTONS_LOCK_HINT     "Si l’appareil est verrouillé, répétez n’importe quel appui maintenu pour le déverrouiller."
#define D_WEB_BUTTONS_ACTION_NONE     "Aucune"
#define D_WEB_BUTTONS_ACTION_BOOKMARK "Marquer la page"
#define D_WEB_BUTTONS_ACTION_LOCK     "Verrouiller l’appareil"
#define D_WEB_BUTTONS_ACTION_MENU     "OK/Ouvrir le menu (selon le contexte)"
#define D_WEB_BUTTONS_ACTION_ROTATE   "Retourner l’orientation de l’écran"
#define D_WEB_BUTTONS_ACTION_NEXT "Élément ou page suivante"
#define D_WEB_BUTTONS_ACTION_PREV "Élément ou page précédente"
#define D_WEB_BUTTONS_ACTION_HOME "Aller au menu principal"
#define D_WEB_BUTTONS_LEGACY_HINT "Si cette option est activée, l’association des clics simple, double et triple n’a aucun effet."

// Click timing section.
#define D_WEB_TIMINGS_HEADING        "Délais des clics"
#define D_WEB_TIMINGS_INTRO          "Réglez le comportement des clics et des appuis maintenus."
#define D_WEB_TIMINGS_GAP_LABEL      "Pause entre clics"
#define D_WEB_TIMINGS_GAP_HINT       "Durée maximale d’une pause entre deux relâchements avant que la série de clics en cours soit validée."
#define D_WEB_TIMINGS_SEQUENCE_LABEL "Durée d’une série"
#define D_WEB_TIMINGS_SEQUENCE_HINT  "Durée totale maximale d’une série de clics, mesurée à partir du premier relâchement."
#define D_WEB_TIMINGS_LONG_LABEL     "Seuil : " D_WEB_BUTTONS_LONG
#define D_WEB_TIMINGS_LONG_HINT      "Durée d’appui à partir de laquelle un maintien devient : " D_WEB_BUTTONS_LONG "."
#define D_WEB_TIMINGS_VLONG_LABEL    "Seuil : " D_WEB_BUTTONS_EXTRA_LONG
#define D_WEB_TIMINGS_VLONG_HINT     "Durée d’appui à partir de laquelle un maintien seul passe de " D_WEB_BUTTONS_LONG " à " D_WEB_BUTTONS_EXTRA_LONG "."
#define D_WEB_TIMINGS_DEBOUNCE_LABEL "Anti-rebond"
#define D_WEB_TIMINGS_DEBOUNCE_HINT  "Les événements plus rapprochés que cette durée sont considérés comme des rebonds du bouton et ignorés."
#define D_WEB_TIMINGS_RESET          "Réinitialiser"
#define D_WEB_TIMINGS_SAVE           "Enregistrer les délais"
#define D_WEB_TIMINGS_DEFAULT_PREFIX "Par défaut : "
// Device personalization card (src/web/settings.cpp).
#define D_WEB_DEVICE_HEADING        "Appareil"
#define D_WEB_DEVICE_INTRO          "Personnalisez le nom de l’appareil affiché en haut de l’écran bibliothèque."
#define D_WEB_HEADER_TITLE_LABEL    "Titre de l’en-tête"
#define D_WEB_HEADER_TITLE_HINT     "Affiché en haut de l’écran bibliothèque. Laissez vide pour ne pas afficher de titre."
#define D_WEB_HEADER_TITLE_RESET    "Rétablir par défaut"
#define D_WEB_FLIP_SCREEN           "Retourner l’orientation de l’écran"
#define D_WEB_BATTERY_INDICATORS_LABEL "Masquer les indicateurs de batterie"
#define D_WEB_BATTERY_INDICATORS_HINT "Si cette case est cochée, l’état de la batterie n’est visible que sur l’écran " D_ABOUT_HEADER "."

// Header status icons card (src/web/settings.cpp) — see src/ui/icons.h.
#define D_WEB_ICONS_HEADING         "Icônes d’état"
#define D_WEB_ICONS_INTRO           "Petits indicateurs affichés dans l’en-tête du menu pour les états qui consomment plus de batterie."
#define D_WEB_ICON_SLEEP_LABEL      "Afficher l’icône de veille bloquée"
#define D_WEB_ICON_SLEEP_HINT       "Une lune barrée, affichée tant que quelque chose empêche l’appareil de se mettre en veille : un ordinateur connecté ou un transfert en cours."

// Wi-Fi card (src/web/settings.cpp) — the saved network the upload screen
// joins.
#define D_WEB_WIFI_HEADING            "Wi-Fi"
#define D_WEB_WIFI_INTRO              "Le réseau que l’appareil rejoint pour le transfert de fichiers. Pour des raisons de sécurité, les mots de passe enregistrés ne sont jamais affichés."
#define D_WEB_WIFI_SSID_LABEL         "SSID"
#define D_WEB_WIFI_PASSWORD_LABEL     "Mot de passe"
#define D_WEB_WIFI_SSID_PLACEHOLDER   "Nom du réseau"
#define D_WEB_WIFI_PASSWORD_PLACEHOLDER "Mot de passe"
#define D_WEB_WIFI_PASSWORD_SAVED_PLACEHOLDER "<enregistré>"
#define D_WEB_WIFI_SHOW_PASSWORD      "Afficher le mot de passe"
#define D_WEB_WIFI_SAVE_BUTTON        "Enregistrer le réseau"
#define D_WEB_WIFI_HINT               "Enregistré sur l’appareil. Laissez le SSID vide pour l’oublier."

// ----------------------------------------------------------------------------
//  Upload (book + sleep image) routes (src/web/upload.cpp)
// ----------------------------------------------------------------------------
#define D_WEB_UPLOAD_COMPLETE_HEADING "Transfert terminé"
#define D_WEB_UPLOAD_COMPLETE_DESC  "Votre livre est maintenant enregistré sur l’appareil et disponible dans la bibliothèque."
#define D_WEB_UPLOAD_BOOK_LABEL     "Livre"
#define D_WEB_UPLOAD_STORED_SIZE    "Taille stockée"
#define D_WEB_UPLOAD_BOOKS_NOW      "Livres au total"
#define D_WEB_UPLOAD_FREE_SPACE     "Espace libre"
#define D_WEB_UPLOAD_ANOTHER        "Envoyer un autre livre"
#define D_WEB_UPLOAD_BOOK_SAVED     "Livre enregistré avec succès."
#define D_WEB_UPLOAD_FINISHED       "&#10003; Transfert terminé."
#define D_WEB_UPLOAD_ERR_FALLBACK   "Échec du transfert"
#define D_WEB_ERR_LIBRARY_FULL      "Bibliothèque pleine"
#define D_WEB_ERR_NOT_ENOUGH_SPACE  "Espace libre insuffisant"
#define D_WEB_ERR_CANT_CREATE_TEMP_BOOK "Impossible de créer le fichier temporaire"
#define D_WEB_ERR_WRITE_FAILED      "Échec de l’écriture (espace insuffisant ?)"
#define D_WEB_ERR_FINALIZE_UPLOAD   "Impossible de finaliser le transfert"
#define D_WEB_ERR_EMPTY_UPLOAD      "Fichier vide"
#define D_WEB_ERR_UPLOAD_ABORTED    "Transfert interrompu"
#define D_WEB_SLEEP_UPLOAD_ERR_FALLBACK "Échec de l’envoi de l’image de veille"
#define D_WEB_SLEEP_UPLOAD_HEADING  "Écran de veille mis à jour"
#define D_WEB_SLEEP_UPLOAD_DESC     "Votre image de veille personnalisée a bien été enregistrée et s’affichera à la prochaine mise en veille de l’appareil."
#define D_WEB_BACK_TO_SETTINGS      "Retour aux réglages"
#define D_WEB_SLEEP_UPLOAD_SUBTITLE "Écran de veille enregistré avec succès."
#define D_WEB_SLEEP_UPLOAD_BANNER   "&#10003; Image de veille personnalisée envoyée."
#define D_WEB_SLEEP_ERR_TEMP        "Impossible de créer le fichier de veille temporaire"
#define D_WEB_SLEEP_ERR_SIZE        "L’image de veille doit faire exactement 3904 octets"
#define D_WEB_SLEEP_ERR_SAVE        "Impossible d’enregistrer l’image de veille"
#define D_WEB_SLEEP_ERR_ABORTED     "Envoi de l’image de veille interrompu"

// ----------------------------------------------------------------------------
//  App upload route (src/web/apps_upload.cpp)
// ----------------------------------------------------------------------------
#define D_WEB_APP_UPLOAD_ERR_FALLBACK "Échec de l’envoi de l’app"
#define D_WEB_APP_INSTALLED_HEADING "App installée"
#define D_WEB_APP_INSTALLED_DESC    "Sur l’appareil, faites défiler la bibliothèque jusqu’à <b>Apps</b> et double-cliquez pour lancer l’app."
#define D_WEB_APP_LABEL             "App"
#define D_WEB_APPS_NOW              "Apps au total"
#define D_WEB_APP_INSTALLED_SUBTITLE "App enregistrée dans /apps/."
#define D_WEB_APP_INSTALLED_BANNER  "&#10003; App prête à être lancée."
#define D_WEB_APP_VALID_OK          "OK"
#define D_WEB_APP_VALID_TOO_SMALL   "App invalide (fichier trop petit)"
#define D_WEB_APP_VALID_BAD_MAGIC   "App invalide (signature incorrecte)"
#define D_WEB_APP_VALID_BAD_ENTRY   "App invalide (point d’entrée incorrect)"
#define D_WEB_APP_VALID_BAD_RELOC   "App invalide (table reloc incorrecte)"
#define D_WEB_APP_VALID_API_FMT     "App invalide (API v%u, requiert v%u)"
#define D_WEB_APP_VALID_INVALID     "App invalide"
#define D_WEB_APPS_DIR_FULL         "Dossier des apps plein"
#define D_WEB_ERR_CANT_CREATE_TEMP_APP "Impossible de créer le fichier app temporaire"
#define D_WEB_APP_TOO_LARGE         "App trop grande (> 48 Ko)"
#define D_WEB_APP_BINARY_TOO_SMALL  "Binaire de l’app trop petit"
#define D_WEB_APP_CANT_READ_HEADER  "Impossible de lire l’en-tête de l’app"
#define D_WEB_APP_FINALIZE_FAILED   "Impossible de finaliser l’envoi de l’app"
#define D_WEB_APP_UPLOAD_ABORTED    "Envoi de l’app interrompu"

// ----------------------------------------------------------------------------
//  Bookmarks web page (src/web/bookmarks.cpp)
// ----------------------------------------------------------------------------
#define D_WEB_BOOKMARKS_HEADING     "Marque-pages"
#define D_WEB_BOOKMARKS_SUBTITLE    "Positions de lecture enregistrées sur le Pala One, regroupées par livre."
#define D_WEB_NO_BOOKS_YET          "Aucun livre disponible pour l’instant."
#define D_WEB_NO_BOOKMARKS_CARD     "Aucun marque-page"
#define D_WEB_BOOKMARKS_OPEN_FAILED_CARD "Ouverture impossible"
#define D_WEB_BOOKMARK_PILL_PREFIX  "Marque-page "
#define D_WEB_BOOKMARK_VIEW         "Voir"
#define D_WEB_CONFIRM_DELETE_BOOKMARK "Supprimer le marque-page ?"
#define D_WEB_BOOKMARK_DOWNLOAD_ALL "Télécharger tous les marque-pages"
#define D_WEB_BOOKMARK_VIEW_HEADING "Aperçu du marque-page"
#define D_WEB_BOOKMARK_VIEW_SUBTITLE "Aperçu du texte de la page enregistrée pour ce marque-page."
#define D_WEB_BOOKMARK_VIEW_BACK_NAV "&#8592; Retour"
#define D_WEB_BOOKMARK_PAGE_EMPTY   "(vide)"
#define D_WEB_BOOKMARK_OPEN_FAILED_DOT "Ouverture impossible."

// Bookmark export plaintext labels (the .txt file downloads).
// Separators (==== / ----) stay verbatim and are NOT translated.
#define D_WEB_BMEXPORT_BOOK         "Livre : "
#define D_WEB_BMEXPORT_BOOKMARKS    "Marque-pages : "
#define D_WEB_BMEXPORT_BOOKMARK_LBL "Marque-page "
#define D_WEB_NO_BOOKMARKS_THIS_BOOK "Aucun marque-page pour ce livre"

// ----------------------------------------------------------------------------
//  In-browser reader + find/jump (src/web/find.cpp).
// ----------------------------------------------------------------------------
#define D_WEB_READ_TITLE            "Lire"
#define D_WEB_READ_SUBTITLE         "Parcourez et recherchez dans le livre depuis votre navigateur. Utilisez Aller ici pour définir le point de reprise sur l’appareil."
#define D_WEB_READ_BYTES_LABEL      "octets"
#define D_WEB_READ_CURRENT_PAGE_LABEL "page actuelle :"
#define D_WEB_READ_FIND_PLACEHOLDER "Rechercher dans le livre"
#define D_WEB_READ_FIND_ALL         "Tout rechercher"
#define D_WEB_READ_FIND_PREV        "Préc."
#define D_WEB_READ_FIND_NEXT        "Suiv."
#define D_WEB_READ_JUMP_HERE        "Aller ici"
#define D_WEB_READ_LOADING          "Chargement du texte..."
#define D_WEB_READ_PAGE_PLACEHOLDER "Numéro de page"
#define D_WEB_READ_JUMP_PAGE        "Aller à la page"
#define D_WEB_READ_JUMP_HINT        "Enregistre directement la page à ouvrir ensuite."
#define D_WEB_READ_AND_FIND_LINK    "Lire &amp; rechercher"

// Status strings emitted by the find/jump JS. These are substituted into
// single-quoted JS string literals inside a PROGMEM <script> block, so the
// no-apostrophe / no-backslash rule from lang.h applies here too. The *_FMT
// values are expanded by window.palaFmt (src/web/chrome.cpp): %1, %2, %3 are
// positional and MAY be reordered by a translation.
#define D_WEB_READ_JS_NO_MATCHES    "Aucun résultat"
#define D_WEB_READ_JS_MATCH_FMT     "Résultat %1 sur %2  (octet %3)"
#define D_WEB_READ_JS_ENTER_PHRASE  "Saisissez une expression à rechercher."
#define D_WEB_READ_JS_FIND_FIRST    "Lancez d’abord une recherche."
#define D_WEB_READ_JS_SAVING        "Enregistrement..."
#define D_WEB_READ_JS_SAVED_FMT     "Enregistré. Ouvrez le livre sur l’appareil pour aller à l’octet %1."
#define D_WEB_READ_JS_SAVE_HTTP_FMT "Échec de l’enregistrement : HTTP %1"
#define D_WEB_READ_JS_SAVE_FAIL_FMT "Échec de l’enregistrement : %1"
#define D_WEB_READ_JS_ERROR         "erreur"
#define D_WEB_READ_JS_LOADED_FMT    "%1 octets chargés. Saisissez une expression à rechercher."
#define D_WEB_READ_JS_LOAD_FAILED   "Impossible de charger le texte du livre."

// ----------------------------------------------------------------------------
//  Font family + bionic reading + reading-position retention
//  (src/web/settings.cpp). Layout-affecting settings; changes trigger the
//  reader to remap its byte-offset cursor under the new layout.
// ----------------------------------------------------------------------------
#define D_WEB_READING_INTRO         "Changer la taille, la police, l’interligne ou le mode bionique conserve votre position dans le livre en cours &mdash; l’appareil recompose les pages autour de l’octet que vous lisez et revient sur la page qui le contient."
#define D_WEB_FONT_FAMILY_LABEL     "Police"
#define D_WEB_FONT_FAMILY_HELVETICA "Helvetica"
#define D_WEB_FONT_FAMILY_DYSLEXIC  "OpenDyslexic"
#define D_WEB_FONT_FAMILY_HINT      "OpenDyslexic utilise des formes de lettres plus marquées, conçues pour faciliter la lecture."
#define D_WEB_BIONIC_LABEL          "Lecture bionique"
#define D_WEB_BIONIC_HINT           "Met en gras les premières lettres de chaque mot pour aider vos yeux à s’ancrer."
#define D_WEB_PARA_GAP_LABEL        "Espacement réduit entre paragraphes"
#define D_WEB_PARA_GAP_HINT         "Si cette option est activée, les sauts de paragraphe sont réduits de moitié, ce qui permet d’afficher plus de lignes sur une page."
#define D_WEB_SETTINGS_APPLY_HINT   "Les changements s’appliquent au prochain affichage de page."
#define D_WEB_SETTINGS_LEGACY_CONTROLS   "Activer les commandes classiques"

// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//  Screensaver editor + multi-slot manager (src/web/screensavers.cpp).
//  The editor's JS status strings are the D_WEB_SS_JS_* block at the end of
//  this section; they are concatenated straight into the PROGMEM script.
// ----------------------------------------------------------------------------
#define D_WEB_SS_TITLE              "Écrans de veille"
#define D_WEB_SS_SUBTITLE           "Images de veille personnalisées, rotation sur plusieurs emplacements et éditeur d’image intégré."
#define D_WEB_SS_ROTATION_HEADING   "Rotation"
#define D_WEB_SS_ROTATION_INTRO     "Choisissez ce qui s’affiche sur l’écran e-ink à chaque mise en veille. Cycle parcourt les emplacements remplis dans l’ordre ; Aléatoire en choisit un au hasard sans répétition immédiate."
#define D_WEB_SS_MODE_LABEL         "Mode"
#define D_WEB_SS_MODE_SINGLE        "Image unique seulement"
#define D_WEB_SS_MODE_CYCLE         "Cycle des emplacements"
#define D_WEB_SS_MODE_SHUFFLE       "Emplacements aléatoires"
#define D_WEB_SS_SLOTS_POPULATED    "Emplacements remplis : "
#define D_WEB_SS_SAVE_MODE          "Enregistrer le mode"
#define D_WEB_SS_SLOTS_HEADING      "Emplacements de rotation"
#define D_WEB_SS_SLOT_LABEL         "Emplacement"
#define D_WEB_SS_SLOT_EMPTY         "vide"
#define D_WEB_SS_CONFIRM_DEL_SLOT   "Supprimer cet emplacement ?"
#define D_WEB_SS_DOWNLOAD_ARIA      "Télécharger l’écran de veille"
#define D_WEB_SS_UPLOAD_ARIA        "Envoyer un écran de veille .bin"
#define D_WEB_SS_DELETE_ARIA        "Supprimer l’écran de veille"
#define D_WEB_SS_ROTATE             "Pivoter de 90\u00b0"
#define D_WEB_SS_SINGLE_HEADING     "Écran de veille unique"
#define D_WEB_SS_SINGLE_ALT         "Écran de veille unique"
#define D_WEB_SS_CONFIRM_DEL_SINGLE "Supprimer l’écran de veille unique ?"
#define D_WEB_SS_NO_SINGLE          "Aucun écran de veille unique envoyé. Utilisez l’éditeur."
#define D_WEB_SS_EDITOR_HEADING     "Éditeur"
#define D_WEB_SS_EDITOR_INTRO       "Déposez une image dans l’éditeur, puis envoyez-la vers un emplacement de rotation ou comme écran de veille unique (ancien mode). Toutes les images sont rendues en 250&times;122, 1 bit (3904 octets)."
#define D_WEB_SS_SOURCE_IMAGE       "Image source"
#define D_WEB_SS_TOLERANCE          "Seuil du noir"
#define D_WEB_SS_INVERT             "Inverser noir/blanc"
#define D_WEB_SS_PRECISE_CONTROL    "Réglage précis"
#define D_WEB_SS_ZOOM               "Zoom"
#define D_WEB_SS_MOVE_X             "Déplacer X"
#define D_WEB_SS_MOVE_Y             "Déplacer Y"
#define D_WEB_SS_PREVIEW_LABEL      "Aperçu (glisser pour déplacer, pincer ou faire défiler pour zoomer)"
#define D_WEB_SS_RESET_FIT          "Réajuster"
#define D_WEB_SS_NO_IMAGE           "Aucune image chargée"
#define D_WEB_SS_SAVE_TO            "Enregistrer dans"
#define D_WEB_SS_DST_SINGLE         "Écran de veille unique (/sleep.bin)"
#define D_WEB_SS_DST_AUTO_PREFIX    "Prochain emplacement libre (emplacement "
#define D_WEB_SS_DST_AUTO_SUFFIX    ")"
#define D_WEB_SS_DST_FULL           "(Tous les emplacements sont pleins)"
#define D_WEB_SS_DST_SLOT_PREFIX    "Emplacement "
#define D_WEB_SS_DST_OVERWRITE      " (remplacer)"
#define D_WEB_SS_UPLOAD_EDITED      "Envoyer l’image modifiée"

// Screensaver upload failures. These are set on the upload handler's error
// slot and returned as the 400 body by handleScreensaverUploadDone.
// EXACT_BYTES_FMT's %d is Screensavers::SCREENSAVER_BYTES.
#define D_WEB_SS_ERR_THUMB_NOT_FOUND "Miniature introuvable"
#define D_WEB_SS_ERR_NOT_FOUND       "Écran de veille introuvable"
#define D_WEB_SS_ERR_UPLOAD_FAILED   "Échec de l’envoi"
#define D_WEB_SS_ERR_SLOTS_FULL      "Tous les emplacements de rotation sont pleins"
#define D_WEB_SS_ERR_CANT_CREATE_TMP "Impossible de créer le fichier temporaire"
#define D_WEB_SS_ERR_IMAGE_TOO_LARGE "Le fichier image est trop volumineux"
#define D_WEB_SS_ERR_CHOOSE_IMAGE    "Choisissez d’abord une image."
#define D_WEB_SS_ERR_EXACT_BYTES_FMT "L’image doit faire exactement %d octets"
#define D_WEB_SS_ERR_SAVE_SLEEP      "Impossible d’enregistrer l’image de veille"
#define D_WEB_SS_ERR_SAVE_SLOT       "Impossible d’enregistrer l’emplacement de rotation"

// Editor JS status strings. Same single-quote / backslash restriction and
// same window.palaFmt positional %N expansion as the D_WEB_READ_JS_* block.
// The "no image" reset reuses D_WEB_SS_NO_IMAGE, which is also the initial
// server-rendered value of the same element.
#define D_WEB_SS_JS_PX_SUFFIX        " px"
#define D_WEB_SS_JS_PREVIEW_FMT      "Aperçu : %1x%2  seuil %3  octets %4"
#define D_WEB_SS_JS_DECODE_FAILED    "Impossible de décoder l’image."
#define D_WEB_SS_JS_READ_FAILED      "Impossible de lire l’image."
#define D_WEB_SS_JS_PREVIEW_NOT_READY "L’aperçu n’est pas encore prêt."
#define D_WEB_SS_JS_UPLOADING        "Envoi en cours..."
#define D_WEB_SS_JS_HTTP_FMT         "HTTP %1"
#define D_WEB_SS_JS_UPLOAD_COMPLETE  "Envoi terminé. Actualisation..."
#define D_WEB_SS_JS_UPLOAD_FAIL_FMT  "Échec de l’envoi : %1"
#define D_WEB_SS_JS_ERROR            "erreur"


#endif  // PALA_LANG_FR_H
