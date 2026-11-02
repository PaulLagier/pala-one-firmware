#ifndef PALA_LANG_ES_LA_H
#define PALA_LANG_ES_LA_H

// ============================================================================
//  Spanish (Latin America) string table — es_LA.
//  Mirror of en.h; the key set MUST stay identical. Adding new keys: edit en.h
//  first, then add the same key here. See src/lang/lang.h for the rule.
//
//  Glyph coverage: all accents used here (á é í ó ú ñ ¿ ¡ ü) are in u8g2's
//  Latin Extended (_te) font set already linked by src/ui/font.cpp, so no
//  font change is required. Web responses already declare charset=utf-8.
// ============================================================================

// ----------------------------------------------------------------------------
//  Boot / fatal screens
// ----------------------------------------------------------------------------
#define D_BOOT_STORAGE_ERROR        "Error de almacenamiento"
#define D_BOOT_TRY_FACTORY_RESET    "Pruebe reinicio de fábrica"

// ----------------------------------------------------------------------------
//  About screen
// ----------------------------------------------------------------------------
#define D_ABOUT_HEADER              "Dispositivo" // AI translation
#define D_ABOUT_FIRMWARE_PREFIX     "Firmware" // AI translation
#define D_ABOUT_GESTURE_CLICK       "clic" // AI translation
#define D_ABOUT_GESTURE_CLICK_2     "clic x 2" // AI translation
#define D_ABOUT_GESTURE_CLICK_3     "clic x 3" // AI translation
#define D_ABOUT_GESTURE_CLICK_HOLD  "clic + mantener" // AI translation
#define D_ABOUT_GESTURE_HOLD        "mantener" // AI translation
#define D_ABOUT_GESTURE_LONG_HOLD   "mantener mucho" // AI translation
#define D_ABOUT_GESTURE_SEPARATOR   ": "

// ----------------------------------------------------------------------------
// Action names  (src/ui/reader_actions.cpp)
// ----------------------------------------------------------------------------
#define D_ACTION_NONE_LABEL     "Ninguna" // AI translation
#define D_ACTION_NEXT_LABEL     "Siguiente" // AI translation
#define D_ACTION_OPEN_LABEL     "Abrir/Seleccionar" // AI translation
#define D_ACTION_BOOKMARK_LABEL "Marcador" // AI translation
#define D_ACTION_HOME_LABEL     "Inicio" // AI translation
#define D_ACTION_LOCK_LABEL     "Bloquear" // AI translation
#define D_ACTION_MENU_LABEL     "Menú" // AI translation
#define D_ACTION_ROTATE_LABEL   "Girar pantalla" // AI translation

// ----------------------------------------------------------------------------
//  Library menu entries
// ----------------------------------------------------------------------------
#define D_MENU_BOOKMARKS            "Marcapáginas"
#define D_MENU_LIST                 "Lista"
#define D_MENU_APPS                 "Apps"
#define D_MENU_SETTINGS             "Configuración de diseño" // Translated by Claude
#define D_MENU_STATISTICS           "Estadísticas"
#define D_MENU_DEVICE               "Dispositivo"
#define D_MENU_UPLOAD               "Conectar"
#define D_LIBRARY_OPEN_FAILED       "Error al abrir"
#define D_LIBRARY_TRY_UPLOAD        "Intente subir de nuevo"

// ----------------------------------------------------------------------------
//  Statistics screen
// ----------------------------------------------------------------------------
#define D_STATS_HEADING                "Estadísticas"
#define D_STATS_STREAK_CURRENT_FMT     "Racha actual: %u días"
#define D_STATS_STREAK_LONGEST_FMT     "Mejor: %u  Sesiones: %u"
#define D_STATS_LIFETIME_PAGES_FMT     "Páginas leídas: %llu"
#define D_STATS_LIFETIME_PRESSES_FMT   "Pulsaciones: %llu"

// ----------------------------------------------------------------------------
//  List screen
// ----------------------------------------------------------------------------
#define D_LIST_HEADER               "Lista"
#define D_LIST_NONE                 "Sin elementos"

// ----------------------------------------------------------------------------
//  Settings screen (src/ui/screens/settings_screen.cpp). The D_WEB_* labels
//  below were shared with the old server-rendered Web UI; the SPA now carries
//  its own strings in webui/i18n/, so they live here for the device screen.
//  All elements translated with Claude.
// ----------------------------------------------------------------------------
#define D_SETTINGS_HEADER           "Configuración de diseño"

#define D_WEB_FONT_SIZE_LABEL       "Tamaño de fuente"
#define D_WEB_LINE_SPACING_LABEL    "Espaciado de línea"
#define D_WEB_FONT_FAMILY_LABEL     "Familia de fuente"
#define D_WEB_FONT_FAMILY_HELVETICA "Helvetica"
#define D_WEB_FONT_FAMILY_DYSLEXIC  "OpenDyslexic"
#define D_WEB_BIONIC_LABEL          "Lectura biónica"

#define D_SETTINGS_GAP_0            "0 - compacto"
#define D_SETTINGS_GAP_1            "1 - normal"
#define D_SETTINGS_GAP_2            "2 - holgado"
#define D_SETTINGS_GAP_3            "3 - amplio"

#define D_SETTINGS_SIZE_8           "8 - diminuto"
#define D_SETTINGS_SIZE_10          "10 - pequeño"
#define D_SETTINGS_SIZE_12          "12 - mediano"
#define D_SETTINGS_SIZE_14          "14 - grande"

// ----------------------------------------------------------------------------
//  Upload screen
// ----------------------------------------------------------------------------
#define D_UPLOAD_HEADER             "Subir"
#define D_UPLOAD_WIFI               "Wi-Fi"
#define D_UPLOAD_PASSWORD           "Contraseña"
#define D_UPLOAD_OPEN               "Abrir"
#define D_UPLOAD_CONNECTING         "Conectando"
#define D_UPLOAD_CONNECTED          "Conectado"
#define D_UPLOAD_HOTSPOT_HINT_L1    "Pulsa el botón para"
#define D_UPLOAD_HOTSPOT_HINT_L2    "usar punto de acceso"

// ----------------------------------------------------------------------------
//  Apps screen
// ----------------------------------------------------------------------------
#define D_APPS_HEADER               "Apps"
#define D_APPS_NONE                 "Sin apps"

// ----------------------------------------------------------------------------
//  Update screen
// ----------------------------------------------------------------------------
#define D_MENU_UPDATE               "Actualización FW"
#define D_UPDATE_HEADER             "Actualización"
#define D_UPDATE_VERSION_PREFIX     "Versión actual: "
#define D_UPDATE_CHANNEL_LABEL      "Seleccionar canal:"
#define D_UPDATE_CHAN_STABLE        "Estable"
#define D_UPDATE_CHAN_DEV           "Dev"
#define D_UPDATE_BTN_CHECK          "[ Buscar actualización ]"
#define D_UPDATE_NO_CREDS_L1        "Sin credenciales Wi-Fi."
#define D_UPDATE_NO_CREDS_L2        "Configura via instalador."
#define D_UPDATE_CONNECTING         "Conectando..."
#define D_UPDATE_CONN_FAILED        "Conexión Wi-Fi fallida"
#define D_UPDATE_CHECKING           "Verificando..."
#define D_UPDATE_SERVER_FAIL        "No se puede alcanzar el servidor"
#define D_UPDATE_UP_TO_DATE         "Ya está actualizado"
#define D_UPDATE_AVAILABLE_PREFIX   "Disponible: "
#define D_UPDATE_BTN_INSTALL        "[ Instalar actualización ]"
#define D_UPDATE_INSTALLING         "Instalando..."
#define D_UPDATE_DOWNLOAD_FAILED    "Descarga fallida"
#define D_UPDATE_REBOOT_MSG         "Actualización instalada"
#define D_UPDATE_REBOOT_HINT        "2x para reiniciar"

// ----------------------------------------------------------------------------
//  Bookmarks screens
// ----------------------------------------------------------------------------
#define D_BOOKMARKS_HEADER          "Marcapáginas"
#define D_BOOKMARKS_NO_BOOKS        "Sin libros"
#define D_BOOKMARKS_NONE            "Sin marcapáginas"
#define D_BOOKMARKS_OPEN_FAILED     "Error al abrir"

// ----------------------------------------------------------------------------
//  Reader
// ----------------------------------------------------------------------------
#define D_READER_BOOK_EMPTY         "Libro vacío"
#define D_READER_BACK_LIBRARY       "Volver a biblioteca"
#define D_READER_INDEXING_TITLE     "Preparando libro..."
#define D_READER_INDEXING_DETAIL    "Reconstruyendo índice de páginas"

// ----------------------------------------------------------------------------
//  Reader menu overlay
// ----------------------------------------------------------------------------
#define D_READER_MENU_HEADING       "Lectura"
#define D_READER_MENU_PAGE_OF_FMT   "Página %d de %d  (%d%%)"
#define D_READER_MENU_PAGE_PCT_FMT  "Página %d  (%d%% del libro)"
#define D_READER_MENU_STATUSBAR_FMT "Barra de estado: %s"
#define D_READER_MENU_HINT          "clic: cambiar  2x: cerrar"
#define D_STATUSBAR_MODE_FULL       "Completa"
#define D_STATUSBAR_MODE_MINIMAL    "Mínima"
#define D_STATUSBAR_MODE_HIDDEN     "Oculta"

// ----------------------------------------------------------------------------
//  Toasts
// ----------------------------------------------------------------------------
#define D_TOAST_SCREEN_FLIPPED      "Pantalla volteada"
#define D_TOAST_STREAK_DAY_FMT      "Racha de lectura: día %u"


// ----------------------------------------------------------------------------
//  App loader error overlay
// ----------------------------------------------------------------------------
#define D_APP_ERR_TITLE             "Error de app"
#define D_APP_ERR_NULL_PATH         "ruta nula"
#define D_APP_ERR_NOT_FOUND         "App no encontrada"
#define D_APP_ERR_TOO_SMALL         "App muy pequeña"
#define D_APP_ERR_INVALID_FILE      "Archivo inválido"
#define D_APP_ERR_TOO_LARGE         "App muy grande"
#define D_APP_ERR_SIZE_LIMIT        "> 48 KB"
#define D_APP_ERR_READ              "Error de lectura"
#define D_APP_ERR_PARTIAL_READ      "Lectura parcial"
#define D_APP_ERR_NO_EXEC_MEM       "Sin memoria ejec."
#define D_APP_ERR_BAD_FILE          "App inválida"
#define D_APP_ERR_WRONG_MAGIC       "Firma incorrecta"
#define D_APP_ERR_API_MISMATCH      "API incompatible"
#define D_APP_ERR_API_FMT           "API v%u, requiere v%u"
#define D_APP_ERR_BAD_ENTRY         "Entrada inválida"
#define D_APP_ERR_BAD_RELOC         "Tabla reloc inválida"
#define D_APP_ERR_RELOC_RANGE       "Reloc fuera de rango"

// ----------------------------------------------------------------------------
//  Bookmark add toasts
// ----------------------------------------------------------------------------
#define D_TOAST_BOOKMARK_EXISTS     "Marcapáginas ya existe"
#define D_TOAST_BOOKMARK_SAVED      "Marcapáginas guardado"

// ----------------------------------------------------------------------------
//  Lock / screensaver
// ----------------------------------------------------------------------------
#define D_SCREENSAVER_LOCKED        "Bloqueado"
#define D_TOAST_UNLOCKED            "Desbloqueado"

// ============================================================================
//  Web UI
// ============================================================================

// ----------------------------------------------------------------------------
//  Plain-text 4xx/5xx error bodies
// ----------------------------------------------------------------------------
#define D_WEB_ERR_MISSING_ID            "id faltante"
#define D_WEB_ERR_BAD_ID                "id inválido"
#define D_WEB_ERR_BAD_FOLDER            "carpeta inválida"
#define D_WEB_ERR_FOLDER_LIMIT          "límite de carpetas alcanzado"
#define D_WEB_ERR_MKDIR_FAILED          "fallo al crear carpeta"
#define D_WEB_ERR_FOLDER_NOT_FOUND      "carpeta no encontrada"
#define D_WEB_ERR_FOLDER_NOT_EMPTY      "carpeta no vacía"
#define D_WEB_ERR_DELETE_FAILED         "fallo al eliminar"
#define D_WEB_ERR_FOLDER_CREATE_FAILED  "fallo al crear carpeta"
#define D_WEB_ERR_DEST_EXISTS           "destino ya existe"
#define D_WEB_ERR_MOVE_FAILED           "fallo al mover"
#define D_WEB_ERR_FILE_NAME_TOO_LONG    "fallo al mover: nuevo ruta del fichero de archivo supera los %d caracteres: %s"
#define D_WEB_ERR_MISSING_NAME          "nombre faltante"
#define D_WEB_ERR_INVALID_NAME          "nombre inválido"
#define D_WEB_ERR_MISSING_OFFSET        "offset faltante"
#define D_WEB_ERR_OPEN_FAILED           "Error al abrir"
#define D_WEB_ERR_NOT_FOUND             "No encontrado"
#define D_WEB_ERR_APP_NOT_FOUND         "App no encontrada"

// ----------------------------------------------------------------------------
//  Book upload route
// ----------------------------------------------------------------------------
#define D_WEB_UPLOAD_ERR_FALLBACK   "Subida fallida"
#define D_WEB_ERR_LIBRARY_FULL      "Biblioteca llena"
#define D_WEB_ERR_NOT_ENOUGH_SPACE  "Espacio insuficiente"
#define D_WEB_ERR_CANT_CREATE_TEMP_BOOK "No se pudo crear archivo temporal de subida"
#define D_WEB_ERR_WRITE_FAILED      "Fallo de escritura (¿sin espacio?)"
#define D_WEB_ERR_FINALIZE_UPLOAD   "Fallo al finalizar la subida"
#define D_WEB_ERR_EMPTY_UPLOAD      "Subida vacía"
#define D_WEB_ERR_UPLOAD_ABORTED    "Subida abortada"

// ----------------------------------------------------------------------------
//  App upload route
// ----------------------------------------------------------------------------
#define D_WEB_APP_UPLOAD_ERR_FALLBACK "Subida de app fallida"
#define D_WEB_APP_VALID_OK          "OK"
#define D_WEB_APP_VALID_TOO_SMALL   "App inválida (archivo muy pequeño)"
#define D_WEB_APP_VALID_BAD_MAGIC   "App inválida (firma incorrecta)"
#define D_WEB_APP_VALID_BAD_ENTRY   "App inválida (entrada inválida)"
#define D_WEB_APP_VALID_BAD_RELOC   "App inválida (tabla reloc inválida)"
#define D_WEB_APP_VALID_API_FMT     "App inválida (API v%u, requiere v%u)"
#define D_WEB_APP_VALID_INVALID     "App inválida"
#define D_WEB_APPS_DIR_FULL         "Directorio de apps lleno"
#define D_WEB_ERR_CANT_CREATE_TEMP_APP "No se pudo crear archivo temporal de app"
#define D_WEB_APP_TOO_LARGE         "App muy grande (> 48 KB)"
#define D_WEB_APP_BINARY_TOO_SMALL  "Binario de app muy pequeño"
#define D_WEB_APP_CANT_READ_HEADER  "No se pudo leer el encabezado de la app"
#define D_WEB_APP_FINALIZE_FAILED   "Fallo al finalizar la subida de la app"
#define D_WEB_APP_UPLOAD_ABORTED    "Subida de app abortada"

// ----------------------------------------------------------------------------
//  Bookmarks API + .txt export
// ----------------------------------------------------------------------------
#define D_WEB_BOOKMARKS_OPEN_FAILED_CARD "Error al abrir"
#define D_WEB_BOOKMARK_PAGE_EMPTY   "(vacío)"
#define D_WEB_BOOKMARK_OPEN_FAILED_DOT "Error al abrir."

// Bookmark export plaintext labels
#define D_WEB_BMEXPORT_BOOK         "Libro: "
#define D_WEB_BMEXPORT_BOOKMARKS    "Marcapáginas: "
#define D_WEB_BMEXPORT_BOOKMARK_LBL "Marcapáginas "
#define D_WEB_NO_BOOKMARKS_THIS_BOOK "Sin marcapáginas para este libro"

// ----------------------------------------------------------------------------
//  Rutas de miniatura / descarga / subida de salvapantallas (src/web/screensavers.cpp).
// ----------------------------------------------------------------------------

// Fallos de subida de protector de pantalla
#define D_WEB_SS_ERR_THUMB_NOT_FOUND "Miniatura no encontrada"
#define D_WEB_SS_ERR_NOT_FOUND       "Protector de pantalla no encontrado"
#define D_WEB_SS_ERR_UPLOAD_FAILED   "Fallo en la subida"
#define D_WEB_SS_ERR_SLOTS_FULL      "Todas las ranuras de rotación están llenas"
#define D_WEB_SS_ERR_CANT_CREATE_TMP "No se pudo crear el archivo temporal"
#define D_WEB_SS_ERR_IMAGE_TOO_LARGE "El archivo de imagen es demasiado grande"
#define D_WEB_SS_ERR_CHOOSE_IMAGE    "Elige una imagen primero."
#define D_WEB_SS_ERR_EXACT_BYTES_FMT "La imagen debe tener exactamente %d bytes"
#define D_WEB_SS_ERR_SAVE_SLEEP      "Fallo al guardar la imagen de reposo"
#define D_WEB_SS_ERR_SAVE_SLOT       "Fallo al guardar la ranura de rotación"

#endif  // PALA_LANG_ES_LA_H
