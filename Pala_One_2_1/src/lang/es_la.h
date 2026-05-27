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
#define D_ABOUT_HEADER              "Dispositivo"
#define D_ABOUT_FIRMWARE_PREFIX     "Firmware "
#define D_ABOUT_GESTURE_NEXT        "1x siguiente / abajo"
#define D_ABOUT_GESTURE_OPEN        "2x abrir / elegir"
#define D_ABOUT_GESTURE_HOME        "3x inicio"
#define D_ABOUT_GESTURE_BOOKMARK    "Mantener: marcapáginas"

// ----------------------------------------------------------------------------
//  Library menu entries
// ----------------------------------------------------------------------------
#define D_MENU_BOOKMARKS            "Marcapáginas"
#define D_MENU_LIST                 "Lista"
#define D_MENU_APPS                 "Apps"
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
//  Shared chrome / storage card
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//  Navigation links
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//  Home page
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//  Files page
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//  Plain-text 4xx/5xx error bodies
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//  List page
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//  Reset page
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//  Settings page
// ----------------------------------------------------------------------------

// Buttons / remappable hold-gestures section.
#define D_WEB_BUTTONS_HEADING       "Botones"
#define D_WEB_BUTTONS_HINT          "1 clic = siguiente, 2 = anterior, 3 = inicio. Las tres pulsaciones largas abajo son reasignables."
#define D_WEB_BUTTONS_LONG          "Pulsación larga"
#define D_WEB_BUTTONS_EXTRA_LONG    "Pulsación muy larga"
#define D_WEB_BUTTONS_CLICK_HOLD    "Clic y mantener"
#define D_WEB_BUTTONS_SAVE          "Guardar botones"
#define D_WEB_BUTTONS_LOCK_HINT     "Si está bloqueado, repita cualquier pulsación larga para desbloquear."
#define D_WEB_BUTTONS_ACTION_NONE     "Ninguna"
#define D_WEB_BUTTONS_ACTION_BOOKMARK "Marcar página"
#define D_WEB_BUTTONS_ACTION_LOCK     "Bloquear dispositivo"
#define D_WEB_BUTTONS_ACTION_MENU     "Abrir menú"

// ----------------------------------------------------------------------------
//  Upload routes
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
//  Bookmarks web page
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
//  Lector en el navegador + buscar/saltar (src/web/find.cpp).
// ----------------------------------------------------------------------------
#define D_WEB_READ_TITLE            "Leer"
#define D_WEB_READ_SUBTITLE         "Explora y busca el libro en tu navegador. Usa Saltar para fijar el punto de retoma del dispositivo."
#define D_WEB_READ_BYTES_LABEL      "bytes"
#define D_WEB_READ_CURRENT_PAGE_LABEL "página actual:"
#define D_WEB_READ_FIND_PLACEHOLDER "Buscar en el libro"
#define D_WEB_READ_FIND_ALL         "Buscar todo"
#define D_WEB_READ_FIND_PREV        "Anterior"
#define D_WEB_READ_FIND_NEXT        "Siguiente"
#define D_WEB_READ_JUMP_HERE        "Saltar aquí"
#define D_WEB_READ_LOADING          "Cargando texto del libro..."
#define D_WEB_READ_PAGE_PLACEHOLDER "Número de página"
#define D_WEB_READ_JUMP_PAGE        "Saltar a página"
#define D_WEB_READ_JUMP_HINT        "Guarda directamente la próxima página de apertura."
#define D_WEB_READ_AND_FIND_LINK    "Leer y buscar"

// ----------------------------------------------------------------------------
//  Familia de fuente + lectura biónica + retención de posición
//  (src/web/settings.cpp).
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//  Tarjeta de salvapantallas en la página de ajustes (src/web/settings.cpp).
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//  Editor y administrador multi-ranura de salvapantallas (src/web/screensavers.cpp).
//  Las cadenas internas del editor en JS (estado / errores) aún NO están i18n'd.
// ----------------------------------------------------------------------------

#endif  // PALA_LANG_ES_LA_H
